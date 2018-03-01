/*#############################################################################
 * �ļ���   : hasher.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��28��
 * �ļ����� : Hasher����
 * ##########################################################################*/

#include "bc_typedef.hpp"
#include "big_number.hpp"
#include "sha1.hpp"

namespace BroadCache
{

INTERFACE Hasher
{
	virtual void Update(const char* data, int len) = 0;
	virtual void Reset() = 0;
	virtual BigNumber Final() = 0;
};

//=============================================================================

class Sha1Hasher : public Hasher
{
public:
	enum { HASH_NUMBER_SIZE = 20 };
	Sha1Hasher();
	Sha1Hasher(const char* data, int len);

	void Update(const char* data, int len);
	void Reset();
	BigNumber Final();

private:
	SHA_CTX context_;
};

}