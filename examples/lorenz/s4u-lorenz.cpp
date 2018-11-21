#include "simgrid/s4u.hpp"
#include "simgrid-fmi.hpp"
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(main, "Messages specific for this msg example");

// SYSTEM'S STATE

const double max_sim_time = 200;

// UTILITY

static int waiter(std::vector<std::string> args){
	XBT_INFO("perform co-simulation of Lorenz system until time 200");
	simgrid::s4u::this_actor::sleep_for(max_sim_time);
	XBT_INFO("co-simulation of Lorenz system done ! see you !");
}


// MAIN

int main(int argc, char *argv[])
{

  // SIMGRID INIT
  simgrid::s4u::Engine e(&argc, argv);


  const double intstepsize = 0.005;
  simgrid::fmi::FMIPlugin::initFMIPlugin(intstepsize);


  e.load_platform("clusters_rennes.xml");

  std::vector<std::string> args;

  // ADDING FMUs

  std::string fmu_uri = "file://./lorenz_x";
  std::string fmu_name = "lorenz_x";
  simgrid::fmi::FMIPlugin::addFMUCS(fmu_uri, fmu_name,false);

  std::string fmu_uri2 = "file://./lorenz_y";
  std::string fmu_name2 = "lorenz_y";
  simgrid::fmi::FMIPlugin::addFMUCS(fmu_uri2, fmu_name2,false);

   std::string fmu_uri3 = "file://./lorenz_z";
   std::string fmu_name3 = "lorenz_z";
   simgrid::fmi::FMIPlugin::addFMUCS(fmu_uri3, fmu_name3,false);

  // CONNECTING FMUS

  simgrid::fmi::FMIPlugin::connectFMU("lorenz_x","x","lorenz_y","x");
  simgrid::fmi::FMIPlugin::connectFMU("lorenz_x","x","lorenz_z","x");
  simgrid::fmi::FMIPlugin::connectFMU("lorenz_y","y","lorenz_x","y");
  simgrid::fmi::FMIPlugin::connectFMU("lorenz_y","y","lorenz_z","y");
  simgrid::fmi::FMIPlugin::connectFMU("lorenz_z","z","lorenz_y","z");

  // LOG OUTPUT
  std::vector<port> ports_to_monitor = {
		  {"lorenz_x","x"},
		  {"lorenz_y","y"},
  	  	  {"lorenz_z","z"}};

	simgrid::fmi::FMIPlugin::configureOutputLog("output.csv", ports_to_monitor);

	simgrid::fmi::FMIPlugin::readyForSimulation();

  // CREATING SIMGRID ACTORS

  simgrid::s4u::Actor::create("waiting_actor", simgrid::s4u::Host::by_name("c-0.rennes"), waiter, args);

  e.run();

}
