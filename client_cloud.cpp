#include "client_util.hpp"

int main(){

	cloud_sys::Client _client("39.106.25.23",9003);
	_client.Start();
	return 0;
}
