/*
	Copyright (c) 2020, Electrux
	All rights reserved.
	Using the BSD 3-Clause license for the project,
	main LICENSE file resides in project's root directory.
	Please read that file and understand the license terms
	before using or altering the project.
*/

#ifndef VM_VARS_BASE_HPP
#define VM_VARS_BASE_HPP

#include <string>

#include <gmpxx.h>
#include "../../../third_party/mpfrxx.hpp"

enum VarTypes
{
	VT_NIL,

	VT_BOOL,
	VT_INT,
	VT_FLT,
	VT_STR,

	// also called struct
	// all the custom types are created from VT_TYPE as base
	VT_TYPE,
	// rest are generated by the type_id() function

	_VT_LAST,
};

class var_base_t
{
	size_t m_type;
	size_t m_idx;
	size_t m_ref;
	bool m_is_type;
public:
	var_base_t( const size_t & type, const size_t & idx, const size_t & ref, const bool is_type );
	virtual ~var_base_t();

	virtual var_base_t * copy( const size_t & idx ) = 0;
	virtual void set( var_base_t * from ) = 0;

	inline size_t type() const { return m_type; }

	inline size_t idx() const { return m_idx; }

	inline void iref() { ++m_ref; }
	inline size_t dref() { if( m_ref > 0 ) --m_ref; return m_ref; }
	inline size_t ref() const { return m_ref; }

	inline bool is_type() const { return m_is_type; }
};

inline void var_iref( var_base_t * & var )
{
	if( var == nullptr ) return;
	var->iref();
}
inline void var_dref( var_base_t * & var )
{
	if( var == nullptr ) return;
	var->dref();
	if( var->ref() == 0 ) {
		delete var;
		var = nullptr;
	}
}

class var_nil_t : public var_base_t
{
public:
	var_nil_t( const size_t & idx );

	var_base_t * copy( const size_t & idx );
	void set( var_base_t * from );
};
#define NIL( x ) static_cast< var_nil_t * >( x )

class var_bool_t : public var_base_t
{
	bool m_val;
public:
	var_bool_t( const bool val, const size_t & idx );

	var_base_t * copy( const size_t & idx );
	bool & get();
	void set( var_base_t * from );
};
#define BOOL( x ) static_cast< var_bool_t * >( x )

class var_int_t : public var_base_t
{
	mpz_class m_val;
public:
	var_int_t( const mpz_class & val, const size_t & idx );

	var_base_t * copy( const size_t & idx );
	mpz_class & get();
	void set( var_base_t * from );
};
#define INT( x ) static_cast< var_int_t * >( x )

class var_flt_t : public var_base_t
{
	mpfr::mpreal m_val;
public:
	var_flt_t( const mpfr::mpreal & val, const size_t & idx );

	var_base_t * copy( const size_t & idx );
	mpfr::mpreal & get();
	void set( var_base_t * from );
};
#define FLT( x ) static_cast< var_flt_t * >( x )

class var_str_t : public var_base_t
{
	std::string m_val;
public:
	var_str_t( const std::string & val, const size_t & idx );

	var_base_t * copy( const size_t & idx );
	std::string & get();
	void set( var_base_t * from );
};
#define STR( x ) static_cast< var_str_t * >( x )

#endif // VM_VARS_BASE_HPP
