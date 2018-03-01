/*#############################################################################
 * �ļ���   : piece_picker.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��22��
 * �ļ����� : PeerRequest����������PeerRequest�ᱻ��������ã���˽��������ŵ�
			  һ���������ļ��У����ٲ���Ҫ�İ���������
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