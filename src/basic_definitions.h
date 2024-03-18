#ifndef BASIC_DEFINITIONS__guard____fdhsiugfdg4gd894gv65fds46f5s496f5sd6
#define BASIC_DEFINITIONS__guard____fdhsiugfdg4gd894gv65fds46f5s496f5sd6

#include<iostream>
#include<cstddef>
#include<exception>

namespace markussecundus::queue_pooling {


	using byte_t = unsigned char;
	using buffersize_t = std::size_t;
	using segment_id_t = std::uint32_t;


	#define dbg(...) ((void)(std::cout << __VA_ARGS__))


}

#endif