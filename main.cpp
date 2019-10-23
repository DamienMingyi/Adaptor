
#include "HfsCommunicator.hpp"
#include "hfs_log.hpp"
#include "xml_parser.hpp"
// #include "hfs_database.hpp"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>

std::shared_ptr<spdlog::logger> A2FLog::logger;

// void test_database() {
//     hfs_database* db = new hfs_database("prodrw", "rwprod", "192.168.1.16:1521/orcl");
//     db->start();
//     ResultSet* rs = db->query("select accountid from accounts_mapping ");
//     while (rs && rs->next()){
//         cout << rs->getString(1) << endl;
//     }
//     db->end();
// }

int main(int argc, char **argv) {
	// test_database();

	if(argc < 2) {
		fprintf(stderr, "usage: %s configFile\n", argv[0]);
		return 0;
	}

	const char *configFile = argv[1];
	
	a2ftool::XmlNode cfg(configFile);
	a2ftool::XmlNode gw_node;
	if (cfg.hasChild("Gateway")) {
		gw_node = cfg.getChild("Gateway");		
	}
	else {
		gw_node = cfg;
	}
	A2FLog::init(gw_node);
	Communicator comm(gw_node);
	comm.run();
	return 0;
}
