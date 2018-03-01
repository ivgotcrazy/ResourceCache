#include "bcrm_client.hpp"

int main(int arg,char * argv[])
{
	if(arg < 2 || arg > 2)
	{
		std::cout <<"error command"<<std::endl;
		return 0;
	}
	
	BroadCache::BcrmClient bct;
	bct.Run(argv[1]);
	return 0;
}
