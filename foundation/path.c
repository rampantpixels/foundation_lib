/* path.c  -  Foundation library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform foundation library in C11 providing basic support data types and
 * functions to write applications and games in a platform-independent fashion. The latest source code is
 * always available at
 *
 * https://github.com/rampantpixels/foundation_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>

#include <string.h>


string_t path_clean( char* path, size_t length, size_t capacity, bool reallocate )
{
	//Since this function is used a lot we want to perform as much operations
	//in place instead of splicing up into a string array and remerge
	char* replace;
	char* inpath;
	char* pathpart;
	size_t inlength, curcapacity, curlength, remain, next, protocollen, up, last_up, prev_up, driveofs;
	bool absolute;

	if( !path )
		return reallocate ? string_clone( STRING_CONST( "" ) ) : (string_t){ 0, 0 };

	inpath = path;
	pathpart = inpath;
	inlength = length;
	curcapacity = capacity;
	protocollen = string_find_string( path, length, STRING_CONST( "://" ), 0 );
	if( protocollen != STRING_NPOS )
	{
		protocollen += 3; //Also skip the "://" separator
		inlength -= protocollen;
		curcapacity -= protocollen;
		pathpart += protocollen;
	}
	else
	{
		protocollen = 0;
	}
	driveofs = 0;

	replace = pathpart;
	next = 0;
	while( ( next = string_find( replace, inlength, '\\', next ) ) != STRING_NPOS )
		replace[next] = '/';

	replace = pathpart;
	next = 0;
	curlength = inlength;
	while( ( next = string_find_string( replace, curlength, STRING_CONST( "/./" ), next ) ) != STRING_NPOS )
	{
		remain = ( curlength - ( next + 2 ) );
		curlength -= 2;
		memmove( pathpart + next + 1, pathpart + next + 3, remain );
	}

	replace = pathpart;
	next = 0;
	while( ( next = string_find_string( replace, curlength, STRING_CONST( "//" ), next ) ) != STRING_NPOS )
	{
		remain = ( curlength - ( next + 2 ) );
		--curlength;
		memmove( pathpart + next + 1, pathpart + next + 2, remain );
	}

	if( string_equal( pathpart, curlength, STRING_CONST( "." ) ) )
		curlength = 0;

	if( curlength > 1 )
	{
		if( ( pathpart[ curlength - 2 ] == '/' ) && ( pathpart[ curlength - 1 ] == '.' ) )
			curlength -= 2;
		if( string_equal( pathpart, curlength, STRING_CONST( "." ) ) )
			curlength = 0;
		if( string_equal( pathpart, curlength, STRING_CONST( "./" ) ) )
		{
			curlength = 1;
			pathpart[0] = '/';
		}
		if( ( curlength > 1 ) && string_equal( pathpart, 2, STRING_CONST( "./" ) ) )
		{
			--curlength;
			memmove( pathpart, pathpart + 1, curlength );
		}
	}

	absolute = path_is_absolute( inpath, curlength );
	if( absolute )
	{
		if( !curlength )
		{
			if( !inlength && reallocate )
			{
				inlength = 2;
				inpath = memory_reallocate( inpath, inlength + protocollen + 1, 0, protocollen + 1 );
				pathpart = inpath + protocollen;
			}
			if( curcapacity )
			{
				*pathpart = '/';
				++curlength;
			}
		}
		else if( ( curlength >= 2 ) && ( pathpart[1] == ':' ) )
		{
			driveofs = 2;

			//Make sure first character is upper case
			if( ( pathpart[0] >= 'a' ) && ( pathpart[0] <= 'z' ) )
				pathpart[0] = ( pathpart[0] - (char)( (int)'a' - (int)'A' ) );

			if( curlength == 2 )
			{
				if( ( curcapacity <= 2 ) && reallocate )
				{
					capacity = inlength + 2 + protocollen + 1;
					inpath = memory_reallocate( inpath, capacity, 0, inlength + protocollen + 1 );
					inlength += 2;
					curcapacity = capacity - protocollen;
					pathpart = inpath + protocollen;
				}
				if( curcapacity > 2 )
				{
					pathpart[2] = '/';
					++curlength;
				}
			}
			else if( pathpart[2] != '/' )
			{
				//splice in slash in weird-format paths (C:foo/bar/...)
				if( ( curcapacity < ( curlength + 1 ) ) && reallocate )
				{
					capacity = curlength + protocollen + 2;
					inpath = memory_reallocate( inpath, capacity, 0, inlength + protocollen + 1 );
					inlength = curlength + 1;
					curcapacity = capacity - protocollen;
					path = inpath + protocollen;
				}
				if( curcapacity >= ( curlength + 1 ) )
				{
					memmove( pathpart + 3, pathpart + 2, curlength + 1 - 2 );
					pathpart[2] = '/';
					++curlength;
				}
			}
		}
		else if( !protocollen && ( pathpart[0] != '/' ) )
		{
			if( ( curcapacity < ( curlength + 1 ) ) && reallocate )
			{
				capacity = curlength + protocollen + 2;
				inpath = memory_reallocate( inpath, capacity, 0, inlength + protocollen + 1 );
				inlength = curlength + 1;
				curcapacity = capacity - protocollen;
				path = inpath + protocollen;
			}
			if( curcapacity >= ( curlength + 1 ) )
			{
				memmove( pathpart + 1, pathpart, curlength + 1 );
				pathpart[0] = '/';
				++curlength;
			}
		}
	}

	//Deal with .. references
	last_up = driveofs;
	while( ( up = string_find_string( pathpart, curlength, STRING_CONST( "/../" ), last_up ) ) != STRING_NPOS )
	{
		if( up >= curlength )
			break;
		if( up == driveofs )
		{
			if( absolute )
			{
				memmove( pathpart + driveofs + 1, pathpart + driveofs + 4, curlength - ( driveofs + 3 ) );
				curlength -= 3;
			}
			else
			{
				last_up = driveofs + 3;
			}
			continue;
		}
		prev_up = string_rfind( pathpart, curlength, '/', up - 1 );
		if( prev_up == STRING_NPOS )
		{
			if( absolute )
			{
				memmove( pathpart, pathpart + up + 3, curlength - up - 2 );
				curlength -= ( up + 3 );
			}
			else
			{
				memmove( pathpart, pathpart + up + 4, curlength - up - 3 );
				curlength -= ( up + 4 );
			}
		}
		else if( prev_up >= last_up )
		{
			memmove( pathpart + prev_up, pathpart + up + 3, curlength - up - 2 );
			curlength -= ( up - prev_up + 3 );
		}
		else
		{
			last_up = up + 1;
		}
	}

	if( curlength > 1 )
	{
		if( pathpart[ curlength - 1 ] == '/' )
		{
			pathpart[ curlength - 1 ] = 0;
			--curlength;
		}
	}

	if( protocollen )
	{
		if( pathpart[0] == '/' )
		{
			if( curlength == 1 )
				curlength = 0;
			else
			{
				memmove( pathpart, pathpart + 1, curlength );
				--curlength;
			}
		}
		curlength += protocollen;
	}

	if( capacity > curlength )
		inpath[curlength] = 0;

	return (string_t){ inpath, curlength };
}


string_const_t path_base_file_name( const char* path, size_t length )
{
	size_t start, end;
	if( !path || ! length)
		return string_const( 0, 0 );
	start = string_find_last_of( path, length, STRING_CONST( "/\\" ), STRING_NPOS );
	end = string_find( path, length, '.', ( start != STRING_NPOS ) ? start : 0 );
	//For "dot" files, i.e files with names like "/path/to/.file", return the dot name ".file"
	if( !end || ( end == ( start + 1 ) ) )
		end = STRING_NPOS;
	if( start != STRING_NPOS )
		return string_substr( path, length, start + 1, ( end != STRING_NPOS ) ? ( end - start - 1 ) : STRING_NPOS );
	return string_substr( path, length, 0, end );
}


string_const_t path_base_file_name_with_directory( const char* path, size_t length )
{
	size_t start, end;
	if( !path || !length )
		return string_const( 0, 0 );
	start = string_find_last_of( path, length, STRING_CONST( "/\\" ), STRING_NPOS );
	end = string_rfind( path, length, '.', STRING_NPOS );
	//For "dot" files, i.e files with names like "/path/to/.file", return the dot name ".file"
	if( !end || ( end == ( start + 1 ) ) || ( ( start != STRING_NPOS ) && ( end < start ) ) )
		end = STRING_NPOS;
	return string_substr( path, length, 0, ( end != STRING_NPOS ) ? end : STRING_NPOS );
}


string_const_t path_file_extension( const char* path, size_t length )
{
	size_t start = string_find_last_of( path, length, STRING_CONST( "/\\" ), STRING_NPOS );
	size_t end = string_rfind( path, length, '.', STRING_NPOS );
	if( ( end != STRING_NPOS ) && ( ( start == STRING_NPOS ) || ( end > start ) ) )
		return string_substr( path, length, end + 1, STRING_NPOS );
	return string_const( 0, 0 );
}


string_const_t path_file_name( const char* path, size_t length )
{
	size_t end = string_find_last_of( path,length, STRING_CONST( "/\\" ), STRING_NPOS );
	if( end == STRING_NPOS )
		return string_const( path, length );
	return string_substr( path, length, end + 1, STRING_NPOS );
}


string_const_t path_directory_name( const char* path, size_t length )
{
	size_t pathprotocol;
	size_t pathstart = 0;
	size_t end = string_find_last_of( path, length, STRING_CONST( "/\\" ), STRING_NPOS );
	if( end == 0 )
		return string_const( "/", 1 );
	if( end == STRING_NPOS )
		return string_const( 0, 0 );
	pathprotocol = string_find_string( path, length, STRING_CONST( "://" ), 0 );
	if( pathprotocol != STRING_NPOS )
		pathstart = pathprotocol +=2; // add two to treat as absolute path
	return string_substr( path, length, pathstart, end - pathstart );
}


string_const_t path_subdirectory_name( const char* path, size_t length, const char* root, size_t root_length )
{
	string_const_t testroot, testpath;
	size_t pathprotocol, rootprotocol;

	testpath = path_directory_name( path, length );
	pathprotocol = string_find_string( testpath.str, testpath.length, STRING_CONST( "://" ), 0 );
	if( pathprotocol != STRING_NPOS )
	{
		testpath.str += pathprotocol + 2; // add two to treat as absolute path
		testpath.length -= pathprotocol + 2;
	}

	testroot = string_const( root, root_length );
	rootprotocol = string_find_string( testroot.str, testroot.length, STRING_CONST( "://" ), 0 );
	if( rootprotocol != STRING_NPOS )
	{
		testroot.str += rootprotocol + 2;
		testroot.length -= rootprotocol + 2;
	}

	if( testpath.length <= testroot.length )
		return string_const( 0, 0 );

	if( ( pathprotocol != STRING_NPOS ) || ( rootprotocol != STRING_NPOS ) )
	{
		if( ( pathprotocol != rootprotocol ) || !string_equal( path, pathprotocol, root, rootprotocol ) )
			return string_const( 0, 0 );
	}

	if( !string_equal( testpath.str, testroot.length, testroot.str, testroot.length ) )
		return string_const( 0, 0 );

	if( testroot.length && ( testroot.str[ testroot.length - 1 ] != '/' ) )
		++testroot.length; //Make returned path relative (skip separator slash)

	return string_substr( testpath.str, testpath.length, testroot.length, STRING_NPOS );
}


string_const_t path_protocol( const char* uri, size_t length )
{
	size_t end = string_find_string( uri, length, STRING_CONST( "://" ), 0 );
	if( end == STRING_NPOS )
		return string_const( 0, 0 );
	return string_substr( uri, length, 0, end );
}


string_t path_merge( const char* first, size_t first_length, const char* second, size_t second_length )
{
	bool endsep, beginsep;
	if( !first_length )
		return string_clone( second, second_length );
	else if( !second_length )
		return string_clone( first, first_length );
	beginsep = ( first[ first_length - 1 ] == '/' ) || ( first[ first_length - 1 ] == '\\' );
	endsep = ( second[0] == '/' ) || ( second[0] == '\\' );
	if( beginsep && endsep )
		return string_concat( first, first_length, second + 1, second_length - 1 );
	else if( beginsep || endsep )
		return string_concat( first, first_length, second, second_length );
	return string_merge_varg( '/', first, first_length, second, second_length, nullptr );
}


string_t path_append( char* base, size_t base_length, size_t base_capacity, bool reallocate, const char* tail, size_t tail_length )
{
	bool beginsep = ( base_length && ( ( base[ base_length - 1 ] == '/' ) || ( base[ base_length - 1 ] == '\\' ) ) );
	bool endsep = ( tail_length && ( ( tail[0] == '/' ) || ( tail[0] == '\\' ) ) );
	if( beginsep && endsep )
		return string_append( base, base_length, base_capacity, reallocate, tail + 1, tail_length - 1 );
	else if( beginsep || endsep )
		return string_append( base, base_length, base_capacity, reallocate, tail, tail_length );
	return string_append_varg( base, base_length, base_capacity, reallocate, STRING_CONST( "/" ), tail, tail_length, nullptr );
}


string_t path_append_varg( char* base, size_t base_length, size_t base_capacity, bool reallocate, const char* tail, size_t tail_length, ... )
{
	va_list list;
	string_t result = path_append( base, base_length, base_capacity, reallocate, tail, tail_length );
	va_start( list, tail_length );
	result = path_append_vlist( result.str, result.length, ( result.length >= base_capacity ) ? result.length + 1 : base_capacity, reallocate, list );
	va_end( list );
	return result;
}


string_t path_append_vlist( char* base, size_t base_length, size_t base_capacity, bool reallocate, va_list list )
{
	va_list clist;
	string_t result = (string_t){ base, base_length };
	va_copy( clist, list );
	while( true )
	{
		ptr = va_arg( clist, void* );
		if( !ptr )
			break;
		psize = va_arg( clist, size_t );
		if( psize )
		{
			result = path_append( result.str, result.length, base_capacity, reallocate, ptr, psize );
			if( result.length >= base_capacity )
				base_capacity = result.length + 1;
		}
	}
	va_end( clist );
	return result;
}


string_t path_prepend( char* tail, size_t tail_length, size_t tail_capacity, bool reallocate, const char* base, size_t base_length )
{
	bool beginsep = ( base_length && ( ( base[ base_length - 1 ] == '/' ) || ( base[ base_length - 1 ] == '\\' ) ) );
	bool endsep = ( tail_length && ( ( tail[0] == '/' ) || ( tail[0] == '\\' ) ) );
	if( beginsep && endsep )
		return string_prepend( tail, tail_length, tail_capacity, reallocate, base, base_length - 1 );
	else if( beginsep || endsep )
		return string_prepend( tail, tail_length, tail_capacity, reallocate, base, base_length );
	return string_prepend_varg( tail, tail_length, tail_capacity, reallocate, STRING_CONST( "/" ), base, base_length, nullptr );
}


string_t path_prepend_varg( char* tail, size_t tail_length, size_t tail_capacity, bool reallocate, const char* base, size_t base_length, ... )
{
	va_list list;
	string_t result = path_prepend( tail, tail_length, tail_capacity, reallocate, base, base_length );
	va_start( list, base_length );
	result = path_prepend_vlist( result.str, result.length, ( result.length >= tail_capacity ) ? result.length + 1 : tail_capacity, reallocate, list );
	va_end( list );
	return result;
}


string_t path_prepend_vlist( char* tail, size_t tail_length, size_t tail_capacity, bool reallocate, va_list list )
{
	va_list clist;
	string_t result = (string_t){ tail, tail_length };
	va_copy( clist, list );
	while( true )
	{
		ptr = va_arg( clist, void* );
		if( !ptr )
			break;
		psize = va_arg( clist, size_t );
		if( psize )
		{
			result = path_append( result.str, result.length, tail_capacity, reallocate, ptr, psize );
			if( result.length >= tail_capacity )
				tail_capacity = result.length + 1;
		}
	}
	va_end( clist );
	return result;
}


bool path_is_absolute( const char* path, size_t length )
{
	if( !path || !length )
		return false;
	if( string_find_string( path, length, STRING_CONST( "://" ), 0 ) != STRING_NPOS )
		return true;
	if( ( path[0] == '/' ) || ( path[0] == '\\' ) )
		return true;
	if( ( length > 1 ) && ( path[1] == ':' ) ) //Skip separator test to treat weird formats like "C:path/to/something" as absolute
		return true;
	return false;
}


string_t path_absolute( char* path, size_t length, size_t capacity, bool reallocate )
{
	string_t abspath;
	size_t up, last, protocollen;
	if( !path_is_absolute( path, length ) )
	{
		string_const_t cwd = environment_current_working_directory();
		abspath = string_prepend( path, length, capacity, STRING_CONST( "/" ), reallocate );
		abspath = string_prepend( abspath.str, abspath.length, capacity > abspath.length ? capacity : abspath.length + 1, cwd.str, cwd.length, reallocate );
		abspath = path_clean( abspath.str, abspath.length, capacity > abspath.length ? capacity : abspath.length + 1, true, reallocate );
	}
	else
	{
		abspath = path_clean( path, length, capacity, true, reallocate );
	}

	protocollen = string_find_string( abspath.str, abspath.length, STRING_CONST( "://" ), 0 );
	if( protocollen != STRING_NPOS )
		protocollen += 3; //Also skip the "://" separator
	else
		protocollen = 0;

	//Deal with /../ references
	while( ( up = string_find_string( abspath.str, abspath.length, STRING_CONST( "/../" ), protocollen ? protocollen - 1 : 0 ) ) != STRING_NPOS )
	{
		if( ( protocollen && ( up == ( protocollen - 1 ) ) ) || ( !protocollen && ( up == 0 ) ) )
		{
			//This moves mem so "prot://../path" ends up as "prot://path"
			//and "/../path" ends up as "/path"
			memmove( abspath.str + protocollen, abspath.str + 3 + protocollen, abspath.length - ( 3 + protocollen ) );
			abspath.length -= 3;
			continue;
		}
		FOUNDATION_ASSERT( up > 0 );
		last = string_rfind( abspath.str, abspath.length, '/', up - 1 );
		if( last == STRING_NPOS )
		{
			//Must be a path like C:/../something since other absolute paths
			last = up;
		}
		memmove( abspath.str + last, abspath.str + 3 + up, abspath.length  - ( 3 + up ) );
		abspath.length -= 3 + ( up - last );
	}

	if( abspath.length >= 3 )
	{
		while( ( abspath.length >= 3 ) && ( abspath.str[length-3] == '/' ) && ( abspath.str[length-2] == '.' ) && ( abspath.str[length-1] == L'.' ) )
		{
			//Step up
			if( abspath.length == 3 )
				abspath.length = 1;
			else
			{
				up = string_rfind( abspath.str, abspath.length, '/', abspath.length - 4 );
				if( up != STRING_NPOS )
					abspath.length = up + 1;
			}
		}
	}

	if( abspath.str )
		abspath.str[abspath.length] = 0;

	return abspath;
}


string_t path_make_temporary( void )
{
	char uintbuffer[18];
	string_const_t tmpdir = environment_temporary_directory();
	string_t filename = string_from_uint_buffer( uintbuffer, 18, random64(), true, 0, '0' );
	return path_merge( tmpdir.str, tmpdir.length, filename.str, filename.length );
}

