/*
	MIT License

	Copyright (c) 2020 Feral Language repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef THREAD_TYPE_HPP
#define THREAD_TYPE_HPP

#include <thread>
#include <future>

#include "../VM/VM.hpp"

class var_thread_t : public var_base_t
{
	std::thread * m_thread;
	std::shared_future< int > * m_res;
	size_t m_id;
	bool m_owner;
public:
	var_thread_t( std::thread * thread, std::shared_future< int > * res,
		      const size_t & src_id, const size_t & idx, const bool & owner = true );
	var_thread_t( std::thread * thread, std::shared_future< int > * res, const size_t & id,
		      const size_t & src_id, const size_t & idx, const bool & owner = true );
	~var_thread_t();

	var_base_t * copy( const size_t & src_id, const size_t & idx );
	void set( var_base_t * from );

	inline std::thread *& get_thread() { return m_thread; }
	inline std::shared_future< int > * get_future() { return m_res; }
	inline size_t get_id() { return m_id; }
};
#define THREAD( x ) static_cast< var_thread_t * >( x )

#endif // THREAD_TYPE_HPP