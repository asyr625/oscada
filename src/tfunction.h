
//OpenSCADA system file: tfunction.h
/***************************************************************************
 *   Copyright (C) 2003-2008 by Roman Savochenko                           *
 *   rom_as@fromru.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
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

#ifndef TFUNCTIONS_H
#define TFUNCTIONS_H

#include <string>
#include <vector>

#include "tmess.h"
#include "tcntrnode.h"
#include <tvariant.h>

using std::string;
using std::vector;

//*************************************************
//* IO                                            *
//*************************************************
class TFunction;

class IO
{
    friend class TFunction;
    public:
	//Data
	enum Type { String, Integer, Real, Boolean, Object };
	enum IOFlgs
	{
	    Default = 0x00,	//Default mode (Input IO)
	    Output  = 0x01,
	    Return  = 0x02,
	    FullText= 0x04
	};

	//Methods
	IO( const char *id, const char *name, IO::Type type, unsigned flgs, const char *def = "",
		bool hide = false, const char *rez = "" );

	IO &operator=( IO &iio );

	const string &id( )	{ return mId; }
	const string &name( )	{ return mName; }
	const Type &type( )	{ return mType; }
	unsigned flg( )		{ return mFlg; }
	const string &def( )	{ return mDef; }
	bool  hide( )		{ return mHide; }
	const string &rez( )	{ return mRez; }

	void setId( const string &val );
	void setName( const string &val );
	void setType( Type val );
	void setFlg( unsigned val );
	void setDef( const string &val );
	void setHide( bool val );
	void setRez( const string &val );

    private:
	//Attributes
	string	mId;
	string	mName;
	Type	mType;
	unsigned mFlg;
	string	mDef;
	bool	mHide;
	string	mRez;

	TFunction *owner;
};

//*************************************************
//* Function abstract object                      *
//*************************************************
class TValFunc;

class TFunction : public TCntrNode
{
    public:
	//Methods
	TFunction( const string &id );
	virtual ~TFunction( );

	TFunction &operator=( TFunction &func );

	const string &id( )		{ return mId; };
	virtual string name( )		{ return ""; }
	virtual string descr( )		{ return ""; }
	bool startStat( )		{ return run_st; }
	int use( )			{ return used.size(); }

	virtual void setStart( bool val )  { run_st = val; }

	//- IO -
	void ioList( vector<string> &list );
	int ioId( const string &id );
	int ioSize( );
	IO *io( int id );
	void ioAdd( IO *io );
	int ioIns( IO *io, int pos );
	void ioDel( int pos );
	void ioMove( int pos, int to );

	virtual void calc( TValFunc *val )	{ }

	void valAtt( TValFunc *vfnc );
	void valDet( TValFunc *vfnc );

	virtual void preIOCfgChange( );
	virtual void postIOCfgChange( );

    protected:
	//Methods
	void cntrCmdProc( XMLNode *opt );       //Control interface command process

	void preDisable( int flag );

	//Attributes
	string		mId;
	bool		run_st;
	TValFunc	*mTVal;

    private:
	//Methods
	string nodeName( )	{ return id(); }

	//Attributes
	Res		f_res;
	vector<IO*>	mIO;
	vector<TValFunc*>	used;
};

//*************************************************
//* TValFunc                                      *
//*************************************************
class TValFunc
{
    public:
	//Methods
	TValFunc( const string &iname = "", TFunction *ifunc = NULL, bool iblk = true, const string &iuser = "root" );
	virtual ~TValFunc( );

	string user( )				{ return mUser; }
	const string &vfName( )			{ return mName; }

	void setUser( const string &iuser )	{ mUser = iuser; }
	void setVfName( const string &inm )	{ mName = inm; }

	void ioList( vector<string> &list );
	int  ioId( const string &id );
	int  ioSize( );
	IO::Type ioType( unsigned id )
	{
	    if( id >= mVal.size() )    throw TError("ValFunc",_("Id or IO %d error!"),id);
	    return mFunc->io(id)->type();
	}
	unsigned ioFlg( unsigned id )
	{
	    if( id >= mVal.size() )    throw TError("ValFunc",_("Id or IO %d error!"),id);
	    return mFunc->io(id)->flg();
	}
	bool ioHide( unsigned id )
	{
	    if( id >= mVal.size() )    throw TError("ValFunc",_("Id or IO %d error!"),id);
	    return mFunc->io(id)->hide();
	}

	//> get IO value
	string	getS( unsigned id );
	int	getI( unsigned id );
	double	getR( unsigned id );
	char	getB( unsigned id );
	TVarObj	*getO( unsigned id );

	//> set IO value
	void setS( unsigned id, const string &val );
	void setI( unsigned id, int val );
	void setR( unsigned id, double val );
	void setB( unsigned id, char val );
	void setO( unsigned id, TVarObj *val );

	//> Blocked values screen
	bool	blk( )			{ return mBlk; }

	//> Dimension controll
	bool	dimens( )		{ return mDimens; }
	void	setDimens( bool set )	{ mDimens = set; }

	//> Calc function
	virtual void calc( const string &user = "" );
	//> Calc time function
	double  calcTm( )		{ return tm_calc; }
	void setCalcTm( double ivl )	{ tm_calc = ivl; }

	//> Attached function
	TFunction *func( )		{ return mFunc; }
	void setFunc( TFunction *func, bool att_det = true );

	virtual void preIOCfgChange( );
	virtual void postIOCfgChange( );

    protected:
	//Data
	struct SVl
	{
	    IO::Type	tp;
	    union
	    {
		ResString *s;
		double r;
		int    i;
		char   b;
		TVarObj	*o;
	    }val;
	};

	//Attributes
	vector<SVl>	mVal;	//pointer to: string, int, double, bool

    private:
	//Methods
	void funcDisConnect( bool det = true );

	//Attributes
	string	mName,		//Value name
		mUser;		//Link to user
	bool	mBlk,		//Blocked values screen
		mDimens;	//Make dimension of the calc time

	double tm_calc;		//Calc time in mikroseconds

	TFunction	*mFunc;
};

#endif //TFUNCTIONS_H
