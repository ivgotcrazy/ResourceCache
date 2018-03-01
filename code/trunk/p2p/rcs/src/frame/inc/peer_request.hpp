/*#############################################################################
 * 文件名   : piece_picker.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月22日
 * 文件描述 : PeerRequest声明，由于PeerRequest会被多个类引用，因此将它单独放到
			  一个单独的文件中，减少不必要的包含和依赖
 * ##########################################################################*/

#ifndef HEADER_PEER_REQUEST
#define HEADER_PEER_REQUEST

namespace BroadCache
{

struct PeerRequest
{
	PeerRequest() : piece(0), start(0), length(0) {}

	PeerRequest(uint64 piece_index, uint64 offset, uint64 len)
		: piece(piece_index), start(offset), length(len) {}

	bool operator==(const PeerRequest& request) const
	{ 
		return ((piece == request.piece) 
			   && (start == request.start) 
			   && (length == request.length)); 
	}

	bool operator!=(const PeerRequest& request) const
	{
		return !operator==(request);
	}

	uint64 piece; 
	uint64 start;
	uint64 length;
};

template<class Stream>
inline Stream& operator<<(Stream& stream, const PeerRequest& request)
{
	stream << "[" << request.piece 
		   << ":" << request.start 
		   << ":" << request.length 
		   << "]";

	return stream;
}

}

#endif