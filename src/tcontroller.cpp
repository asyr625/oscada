
//OpenSCADA system file: tcontroller.cpp
/***************************************************************************
 *   Copyright (C) 2003-2006 by Roman Savochenko                           *
 *   rom_as@fromru.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "tsys.h"
#include "tmess.h"
#include "ttiparam.h"
#include "tcontroller.h"

//==== TController ====
TController::TController( const string &id_c, const string &daq_db, TElem *cfgelem ) :
    m_db(daq_db), TConfig(cfgelem), run_st(false), en_st(false),
    m_id(cfg("ID").getSd()), m_name(cfg("NAME").getSd()), m_descr(cfg("DESCR").getSd()),
    m_aen(cfg("ENABLE").getBd()), m_astart(cfg("START").getBd())
{
    m_id = id_c;
    m_prm = grpAdd("prm_");
}

TController::~TController(  )
{
    nodeDelAll();
}

void TController::preDisable(int flag)
{
    if( startStat() ) 	stop( );
    if( enableStat() )  disable( );
}

void TController::postDisable(int flag)
{
    try
    {
        if( flag )
	{
	    //Delete DB record
	    SYS->db().at().dataDel(fullDB(),owner().nodePath()+"DAQ",*this);

	    //Delete parameter's tables
            for(unsigned i_tp = 0; i_tp < owner().tpPrmSize(); i_tp++)
	    {
		string tbl = DB()+"."+cfg(owner().tpPrmAt(i_tp).BD()).getS();
		SYS->db().at().open(tbl);
		SYS->db().at().close(tbl,true);
	    }
	}
    }catch(TError err)
    { mess_err(err.cat.c_str(),"%s",err.mess.c_str()); }
}

string TController::name()   
{ 
    return (m_name.size())?m_name:id();
}

string TController::tbl( )
{
    return owner().owner().subId()+"_"+owner().modId();
}

void TController::load( )
{
    mess_info(nodePath().c_str(),_("Load controller's configs!"));

    SYS->db().at().dataGet(fullDB(),owner().nodePath()+"DAQ",*this);

    //Load parameters if enabled
    if( en_st )	LoadParmCfg( );
}

void TController::save( )
{
    mess_info(nodePath().c_str(),_("Save controller's configs!"));

    //Update type controller bd record
    SYS->db().at().dataSet(fullDB(),owner().nodePath()+"DAQ",*this);

    //Save parameters if enabled
    if( en_st ) SaveParmCfg( );
}

void TController::start( )
{
    //Enable if no enabled
    if( run_st ) return;
    if( !en_st ) enable();

    mess_info(nodePath().c_str(),_("Start controller!"));

    //Start for children
    start_();
    
    run_st = true;
}

void TController::stop( )
{
    if( !run_st ) return;

    mess_info(nodePath().c_str(),_("Stop controller!"));

    //Stop for children
    stop_();
    
    run_st = false;
}

void TController::enable( )
{
    if( en_st )	return;

    mess_info(nodePath().c_str(),_("Enable controller!"));

    //Load parameters
    LoadParmCfg( );

    //Enable for children
    enable_();
    
    //Enable parameters
    vector<string> prm_list;
    list(prm_list);
    for( int i_prm = 0; i_prm < prm_list.size(); i_prm++ )
        if( at(prm_list[i_prm]).at().toEnable() )
        try{ at(prm_list[i_prm]).at().enable(); }
        catch(TError err)
        {
            mess_warning(err.cat.c_str(),"%s",err.mess.c_str());
            mess_warning(nodePath().c_str(),_("Enable parameter <%s> error."),prm_list[i_prm].c_str());
        }

    //Set enable stat flag
    en_st=true;
}

void TController::disable( )
{
    if( !en_st ) return;
    //Stop if runed
    if( run_st ) stop();

    mess_info(nodePath().c_str(),_("Disable controller!"));

    //Disable parameters
    vector<string> prm_list;
    list(prm_list);
    for( int i_prm = 0; i_prm < prm_list.size(); i_prm++ )
	if( at(prm_list[i_prm]).at().enableStat() )
        try{ at(prm_list[i_prm]).at().disable(); }
        catch(TError err)
        {
            mess_warning(err.cat.c_str(),"%s",err.mess.c_str());
            mess_warning(nodePath().c_str(),_("Disable parameter <%s> error."),prm_list[i_prm].c_str());
        }

    //Disable for children
    disable_();

    //Free all parameters
    //FreeParmCfg();

    //Clear enable flag
    en_st = false;
}

void TController::LoadParmCfg(  )
{
    //Search and create new parameters
    for( int i_tp = 0; i_tp < owner().tpPrmSize(); i_tp++ )
    {
	try
	{
    	    TConfig c_el(&owner().tpPrmAt(i_tp));
	    
	    int fld_cnt = 0;
	    while( SYS->db().at().dataSeek(DB()+"."+cfg(owner().tpPrmAt(i_tp).BD()).getS(),
					   owner().nodePath()+cfg(owner().tpPrmAt(i_tp).BD()).getS(),fld_cnt++,c_el) )
    	    {
    		try
		{ 
		    if( !present(c_el.cfg("SHIFR").getS()) ) 
			add( c_el.cfg("SHIFR").getS(), i_tp ); 
		}
		catch(TError err) 
		{ 
		    mess_err(err.cat.c_str(),"%s",err.mess.c_str()); 
		    mess_err(nodePath().c_str(),_("Add parameter <%s> error."),c_el.cfg("SHIFR").getS().c_str());
		}
		c_el.cfg("SHIFR").setS("");
    	    }	    
	}catch(TError err) 
	{ 
	    mess_err(err.cat.c_str(),"%s",err.mess.c_str()); 
	    mess_err(nodePath().c_str(),_("Search and create new parameters error."));
	}
    }
    
    //Load present parameters
    vector<string> prm_ls;
    list(prm_ls);
    for( int i_p = 0; i_p < prm_ls.size(); i_p++ )
	at(prm_ls[i_p]).at().load( );	
}

void TController::SaveParmCfg(  )
{
    vector<string> c_list;

    list(c_list);
    for( unsigned i_ls = 0, i_bd=0; i_ls < c_list.size(); i_ls++)
        at(c_list[i_ls]).at().save();
}

void TController::FreeParmCfg(  )
{
    vector<string> c_list;
    list(c_list);
    for( unsigned i_ls = 0; i_ls < c_list.size(); i_ls++)
        del( c_list[i_ls] );
}

void TController::add( const string &name, unsigned type )
{
    if( chldPresent(m_prm,name) ) return;
    chldAdd(m_prm,ParamAttach( name, type ));
}

TParamContr *TController::ParamAttach( const string &name, int type)
{
    return new TParamContr(name, &owner().tpPrmAt(type));
}

void TController::cntrCmdProc( XMLNode *opt )
{
    //Get page info
    if( opt->name() == "info" )
    {
    	ctrMkNode("oscada_cntr",opt,-1,"/",_("Controller: ")+name());
	ctrMkNode("branches",opt,-1,"/br","",0444);	
	if(ctrMkNode("area",opt,-1,"/cntr",_("Controller")))
	{
	    if(ctrMkNode("area",opt,-1,"/cntr/st",_("State")))
	    {
		ctrMkNode("fld",opt,-1,"/cntr/st/en_st",_("Enable"),0664,"root","root",1,"tp","bool");
		ctrMkNode("fld",opt,-1,"/cntr/st/run_st",_("Run"),0664,"root","root",1,"tp","bool");
		ctrMkNode("fld",opt,-1,"/cntr/st/db",_("Controller DB (module.db)"),0660,"root","root",1,"tp","str");
	    }
	    if(ctrMkNode("area",opt,-1,"/cntr/cfg",_("Config")))
	    {
		ctrMkNode("comm",opt,-1,"/cntr/cfg/load",_("Load"),0660);
		ctrMkNode("comm",opt,-1,"/cntr/cfg/save",_("Save"),0660);
		TConfig::cntrCmdMake(opt,"/cntr/cfg",0,"root","root",0664);
	    }
	}
    	if( owner().tpPrmSize() )
	{
	    ctrMkNode("grp",opt,-1,"/br/prm_",_("Parameter"),0440,"root","root",1,"list","/prm/prm");
     	    if(ctrMkNode("area",opt,-1,"/prm",_("Parameters")))
	    {
		if( owner().tpPrmSize() > 1 )
		    ctrMkNode("fld",opt,-1,"/prm/t_prm",_("To add parameters"),0660,"root","root",3,"tp","str","dest","select","select","/prm/t_lst");
		ctrMkNode("list",opt,-1,"/prm/prm",_("Parameters"),0660,"root","root",4,"tp","br","idm","1","s_com","add,del","br_pref","prm_");
	    }
	}
        return;
    }
    //Process command to page
    string a_path = opt->attr("path");
    if( a_path == "/prm/t_prm" && owner().tpPrmSize() )
    {
	if( ctrChkNode(opt,"get",0660,"root","root",SEQ_RD) )	
	    opt->text(TBDS::genDBGet(owner().nodePath()+"addType",owner().tpPrmAt(0).name(),opt->attr("user")));
	if( ctrChkNode(opt,"set",0660,"root","root",SEQ_WR) )	
	    TBDS::genDBSet(owner().nodePath()+"addType",opt->text(),opt->attr("user"));
    }
    else if( a_path == "/prm/prm" && owner().tpPrmSize() )
    {
	if( ctrChkNode(opt,"get",0660,"root","root",SEQ_RD) )
	{
	    vector<string> c_list;
    	    list(c_list);
    	    for( unsigned i_a=0; i_a < c_list.size(); i_a++ )
        	opt->childAdd("el")->attr("id",c_list[i_a])->text(at(c_list[i_a]).at().name());
	}
	if( ctrChkNode(opt,"add",0660,"root","root",SEQ_WR) )
	{
	    add(opt->attr("id"),owner().tpPrmToId(TBDS::genDBGet(owner().nodePath()+"addType",owner().tpPrmAt(0).name(),opt->attr("user"))));
	    at(opt->attr("id")).at().name(opt->text());
	}
	if( ctrChkNode(opt,"del",0660,"root","root",SEQ_WR) )	del(opt->attr("id"),true);
    }
    else if( a_path == "/prm/t_lst" && owner().tpPrmSize() && ctrChkNode(opt,"get",0444) )
    {
	for( unsigned i_a=0; i_a < owner().tpPrmSize(); i_a++ )
	    opt->childAdd("el")->attr("id",owner().tpPrmAt(i_a).name())->text(owner().tpPrmAt(i_a).lName());
    }	
    else if( a_path == "/cntr/st/db" )
    {
	if( ctrChkNode(opt,"get",0660,"root","root",SEQ_RD) )	opt->text(m_db);
	if( ctrChkNode(opt,"set",0660,"root","root",SEQ_WR) )	m_db = opt->text();
    }
    else if( a_path == "/cntr/st/en_st" )
    {
	if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )	opt->text(en_st?"1":"0");
	if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	atoi(opt->text().c_str())?enable():disable();
    }
    else if( a_path == "/cntr/st/run_st" )
    {	
	if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )	opt->text(run_st?"1":"0");
	if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	atoi(opt->text().c_str())?start():stop();
    }
    else if( a_path == "/cntr/cfg/load" && ctrChkNode(opt,"set",0660,"root","root",SEQ_WR) )	load();
    else if( a_path == "/cntr/cfg/save" && ctrChkNode(opt,"set",0660,"root","root",SEQ_WR) )	save();
    else if( a_path.substr(0,9) == "/cntr/cfg" )	TConfig::cntrCmdProc(opt,TSYS::pathLev(a_path,2),"root","root",0664);
}
