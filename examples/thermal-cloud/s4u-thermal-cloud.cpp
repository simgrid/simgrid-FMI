#include "simgrid/plugins/energy.h"
#include "simgrid/s4u.hpp"
#include "simgrid-fmi.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "simgrid/plugins/live_migration.h"
#include <string>
#include <iostream>
#include <fstream>

XBT_LOG_NEW_DEFAULT_CATEGORY(main, "Messages specific for this msg example");

// SYSTEM'S STATE

std::vector<simgrid::s4u::VirtualMachine*> running_vms;

const int nb_hosts_per_cluster = 129;
const int nb_cores_per_host = 6;
const int nb_simultaneous_deployments = 3;
const double vm_creation_period = 10;
const double slave_flop_works = std::numeric_limits<double>::max();
const int vm_core_nb = 3;
const int max_vms = nb_hosts_per_cluster * nb_cores_per_host / vm_core_nb;
const double vm_ram_size = 1e9;

int current_host = 0;
int vm_id = 0;

// format = {time;pow_rennes;pow_sophia;chiller_status;power_supply_status;Q_load_DC;P_other_DC;Q_other_DC;P_chiller;Q_cooling;P_DC;Q_DC;T_R_out}
std::ofstream output;


// UTILITY

class Utility{

public:

	static void logOutput(){

		output << simgrid::s4u::Engine::get_clock() << ";";

		output << getDCPowerConsumption("rennes") << ";";
		output << getDCPowerConsumption("sophia") << ";";

		output << simgrid::fmi::FMIPlugin::getIntegerOutput("chiller_failure","chiller_status") << ";";

		output << simgrid::fmi::FMIPlugin::getIntegerOutput("thermal_system","power_supply_status");

		std::vector<std::string> realOutputNames = {"Q_load_DC","P_other_DC","Q_other_DC","P_chiller","Q_cooling","P_DC","Q_DC","T_R_out"};

		for(std::string var : realOutputNames){
			output << ";" << simgrid::fmi::FMIPlugin::getRealOutput("thermal_system",var);
		}

		output << "\n";
	}

	static void updatePowerInFMUs(){
		double power = getDCPowerConsumption("rennes");
		simgrid::fmi::FMIPlugin::setRealInput("thermal_system","P_load_DC",power);
		double q_cooling = simgrid::fmi::FMIPlugin::getRealOutput("thermal_system","Q_cooling");
		simgrid::fmi::FMIPlugin::setRealInput("chiller_failure","chiller_load",q_cooling);
		logOutput();
		XBT_INFO("set inputs of the FMUs : P_load = %f, chiller_load = %f",power,q_cooling);
	}

	static double getDCPowerConsumption(std::string dc_name){
		double total_power = 0;

		for(int i=0;i<nb_hosts_per_cluster;i++){
			std::string host_name = "c-" + std::to_string(i) + "."+ dc_name;
			simgrid::s4u::Host* host = simgrid::s4u::Host::by_name(host_name);
			total_power += sg_host_get_current_consumption(host);
		}
		return total_power;
	}
};

// EVENT DETECTORS

static bool reactOnZeroValue(std::vector<std::string> args){
	return simgrid::fmi::FMIPlugin::getIntegerOutput(args[0],args[1]) == 0;
}

// EVENT CALLBACKS
static void manageFailure(std::vector<std::string> args){

	// first, we propagate the event to the other FMUs
	int chiller_status = simgrid::fmi::FMIPlugin::getIntegerOutput("chiller_failure","chiller_status");
	simgrid::fmi::FMIPlugin::setIntegerInput("thermal_system","chiller_status",chiller_status);
	Utility::logOutput();

	// then, we wake up the actor
	unsigned long pid_on = std::stoul(args[2],0,10);
	simgrid::s4u::Actor::by_pid(pid_on)->resume();
}


static void wakeUpActor(std::vector<std::string> args){

	Utility::logOutput();
	// then, we wake up the actor
	unsigned long pid_on = std::stoul(args[2],0,10);
	simgrid::s4u::Actor::by_pid(pid_on)->resume();
}



// BEHAVIORS

static void shutDownRennesHosts(std::vector<std::string> args){


	unsigned long pid = simgrid::s4u::Actor::self()->get_pid();
	std::vector<std::string> args_shutdown = {"thermal_system","power_supply_status",std::to_string(pid)};
	simgrid::fmi::FMIPlugin::registerEvent(reactOnZeroValue,wakeUpActor,args_shutdown);
	simgrid::s4u::Actor::self()->suspend();

	// TODO: remove these two lines when the simulation end properly
	Utility::logOutput();
	output.close();

	double T_R_out = simgrid::fmi::FMIPlugin::getRealOutput("thermal_system","T_R_out");
	XBT_INFO("shutting-down rennes DC because the room temperature is too high ( %f °C )",T_R_out);

	for(int i=0;i<nb_hosts_per_cluster;i++){
		std::string host_name = "c-" + std::to_string(i) + ".rennes";
		simgrid::s4u::Host* host = simgrid::s4u::Host::by_name(host_name);
		if(host->is_on())
			host->turn_off();
	}

	unsigned long pid_on = std::stoul(args[0],0,10);
	simgrid::s4u::Actor::by_pid(pid_on)->kill();

	Utility::updatePowerInFMUs();

	XBT_INFO("rennes DC is shutdown");
	//output.close();

}

static int slave_behavior(std::vector<std::string> /*args*/){
  simgrid::s4u::Actor::self()->daemonize();
  simgrid::s4u::this_actor::execute(slave_flop_works);
  return 0;
}

static int master_nominal_behavior(std::vector<std::string> args){

	while(running_vms.size()<max_vms){

		for(int i=0;i<nb_simultaneous_deployments;i++){
			if(running_vms.size()>0){
				vm_id++;
				if(vm_id==nb_cores_per_host/vm_core_nb){
					vm_id=0;
					current_host++;
				}
			}

			std::string vm_name = "vm_" + std::to_string(current_host) + "_" + std::to_string(vm_id);
			std::string host_name = "c-" + std::to_string(current_host) + ".rennes";
			simgrid::s4u::Host* host = simgrid::s4u::Host::by_name(host_name);

			simgrid::s4u::VirtualMachine* vm = new simgrid::s4u::VirtualMachine(vm_name.c_str(), host, vm_core_nb);
			vm->set_ramsize(vm_ram_size);
			vm->start();

			for(int j=0;j<vm_core_nb;j++){
				std::string actor_name = std::to_string(j) + "_" + vm_name;
				simgrid::s4u::Actor::create(vm_name.c_str(), vm , slave_behavior, args);
			}

			running_vms.push_back(vm);

			XBT_INFO("deploying vm %s in host %s",vm_name.c_str(),host_name.c_str());

		}


		Utility::updatePowerInFMUs();

		simgrid::s4u::this_actor::sleep_for(vm_creation_period);
	}

	return 0;
}

static int failureNotifier(std::vector<std::string> args){

	unsigned long pid = simgrid::s4u::Actor::self()->get_pid();

	std::vector<std::string> params = {"chiller_failure","chiller_status",std::to_string(pid)};

	simgrid::fmi::FMIPlugin::registerEvent(reactOnZeroValue,manageFailure,params);
	simgrid::s4u::Actor::self()->suspend();

	XBT_INFO("failure of the chiller detected!!! send a message to notify the failure manager ");
	double* payload = new double();
	*payload = simgrid::s4u::Engine::get_clock();

	simgrid::s4u::Mailbox::by_name("emergency")->put(payload,200);

	return 0;
}


static int failureManager(std::vector<std::string> args){

	simgrid::s4u::Mailbox::by_name("emergency")->get();

	unsigned long pid_current_manager = std::stoul(args[0],0,10);

	// stop deploying VMs you @%^&"A§* !!!!
	simgrid::s4u::Actor::by_pid(pid_current_manager)->kill();

	// start shutting down unused PM
	for(int i=current_host+1;i<nb_hosts_per_cluster;i++){
		std::string host_name = "c-" + std::to_string(i) + ".rennes";
		simgrid::s4u::Host* host = simgrid::s4u::Host::by_name(host_name);
		if(host->is_on())
			host->turn_off();
	}
	Utility::updatePowerInFMUs();

	// try to migrate as VM as possible to the other cluster
	current_host = 0;
	vm_id = 0;
	for(simgrid::s4u::VirtualMachine* vm : running_vms){
		std::string dest_name = "c-" + std::to_string(current_host) + ".sophia";
		simgrid::s4u::Host* dest = simgrid::s4u::Host::by_name(dest_name);

		XBT_INFO("start migration of vm %s to host %s",vm->get_name().c_str(),dest->get_name().c_str());
		sg_vm_migrate(vm, dest);
		XBT_INFO("end of migration of vm %s to host %s",vm->get_name().c_str(),dest_name.c_str());

		vm_id++;

		if(vm_id==nb_cores_per_host/vm_core_nb){
			std::string host_name = "c-" + std::to_string(current_host) + ".rennes";
			simgrid::s4u::Host* host = simgrid::s4u::Host::by_name(host_name);
			XBT_INFO("turning OFF host %s which is empty",host_name.c_str());
			host->turn_off();
			current_host++;
			vm_id=0;
		}

		Utility::updatePowerInFMUs();

	}

	simgrid::s4u::this_actor::sleep_for(100000);

	XBT_INFO("everybody is safe!");

	return 0;

}

// MAIN

int main(int argc, char *argv[])
{

  // OUTPUT SETTING
	output.open("output.csv", std::ios::out);

  // SIMGRID INIT

  sg_host_energy_plugin_init();
  sg_vm_live_migration_plugin_init();
  simgrid::s4u::Engine e(&argc, argv);


  const double intstepsize = 0.01;
  simgrid::fmi::FMIPlugin::initFMIPlugin(intstepsize);


  e.load_platform("clusters_rennes.xml");

  std::vector<std::string> args;

  // ADDING FMU-CS 1.0

  std::string fmu_uri = "file:///home/mecsyco/Documents/chiller_failure";
  std::string fmu_name = "chiller_failure";

  simgrid::fmi::FMIPlugin::addFMUCS(fmu_uri, fmu_name);

  // ADDING FMU-CS v2.0

  std::string fmu_uri_2 = "file:///home/mecsyco/Documents/fmu_test/thermal_system_om";
  std::string fmu_name_2 = "thermal_system";

  simgrid::fmi::FMIPlugin::addFMUCS(fmu_uri_2, fmu_name_2);

  simgrid::fmi::FMIPlugin::setIntegerInput("thermal_system","chiller_status",1);

  Utility::updatePowerInFMUs();

  simgrid::s4u::ActorPtr master_nominal = simgrid::s4u::Actor::create("master_nominal", simgrid::s4u::Host::by_name("c-0.sophia"), master_nominal_behavior, args);
  simgrid::s4u::Actor::create("failure_notifier", simgrid::s4u::Host::by_name("c-0.rennes"), failureNotifier, args);
  std::vector<std::string> args_failure_manager = {std::to_string(master_nominal.get()->get_pid())};
  simgrid::s4u::ActorPtr failure_manager = simgrid::s4u::Actor::create("failure_manager", simgrid::s4u::Host::by_name("c-0.sophia"), failureManager, args_failure_manager);

  std::vector<std::string> args_shutdown = {std::to_string(failure_manager.get()->get_pid())};
  simgrid::s4u::Actor::create("shutdown_process", simgrid::s4u::Host::by_name("c-0.sophia"), shutDownRennesHosts, args_shutdown);

  e.run();

}
