#include "simgrid/s4u.hpp"
#include "simgrid/plugins/fmi.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(main, "Messages specific for this msg example");

/**
 * Event detection functions
 */

static bool reactOnNegativeValue(std::vector<std::string> args){
	return simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getRealOutputs("x") < 0;
}

static bool reactOnPositiveValue(std::vector<std::string> args){
	return simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getBoolOutputs("positive");
}

static bool reactOnNewCycle(std::vector<std::string> args){

	int current_cycle = std::stoi(args[0],0,10);
	return simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getIntegerOutputs("cycles") != current_cycle;

}

/**
 * Event handlers
 */

static void wakeUpCycleCounter(std::vector<std::string> args){
	unsigned long pid_on = std::stoul(args[1],0,10);
	simgrid::s4u::Actor::byPid(pid_on)->resume();
}

static void switchActors(std::vector<std::string> args){

	unsigned long pid_on = std::stoul(args[0],0,10);
	unsigned long pid_off = std::stoul(args[1],0,10);
	std::string type = args[2];

	simgrid::s4u::Actor::byPid(pid_on)->resume();
	simgrid::s4u::Actor::byPid(pid_off)->suspend();

	if(type == "positive"){
		simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->registerEvent(reactOnNegativeValue,switchActors, {args[1],args[0], "negative"});
	}else{
		simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->registerEvent(reactOnPositiveValue,switchActors,{args[1],args[0], "positive"});
	}
}


/**
 * Actor behaviors
 */

static int sampler(std::vector<std::string> args)
{
  if(args[0]=="negative"){
	  simgrid::s4u::Actor::self()->suspend();
  }

  double time = simgrid::s4u::Engine::getClock();
  double time_step = 1.0;

  while(time < 30.0 ){

	  double x = simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getRealOutputs("x");
	  int cycles = simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getIntegerOutputs("cycles");
	  bool positive = simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getBoolOutputs("positive");
	  XBT_INFO("%f;%f;%i;%d",time,x,cycles,positive);

	  simgrid::s4u::this_actor::sleep_for(time_step);

	  time = simgrid::s4u::Engine::getClock();
  }
  simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->deleteEvents();

  return 0;
}


static int cycleCounter(std::vector<std::string> args){

	unsigned long pid = simgrid::s4u::Actor::self()->getPid();
	int cycles = simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getIntegerOutputs("cycles");

	while(1){

		std::vector<std::string> params = {std::to_string(cycles), std::to_string(pid)};

		simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->registerEvent(reactOnNewCycle,wakeUpCycleCounter,params);
		simgrid::s4u::Actor::self()->suspend();
		cycles = simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getIntegerOutputs("cycles");

		XBT_INFO("we complete a cycle, total number = %d",cycles);
	}

	return 0;
}

static int controller(std::vector<std::string> /*args*/){

	simgrid::s4u::this_actor::sleep_for(10.5);

	double time = simgrid::s4u::Engine::getClock();
	double x = simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getRealOutputs("x");
	int cycles = simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getIntegerOutputs("cycles");
	bool positive = simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getBoolOutputs("positive");
	XBT_INFO("%f;%f;%i;%d",time,x,cycles,positive);

	XBT_INFO("changing input omega");
	simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->setRealInput("omega",1.0);

	x = simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getRealOutputs("x");
	cycles = simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getIntegerOutputs("cycles");
	positive = simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->getBoolOutputs("positive");
	XBT_INFO("%f;%f;%i;%d",time,x,cycles,positive);

	return 0;
}

int main(int argc, char *argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  std::vector<std::string> pos_args = {"positive"} ;
  std::vector<std::string> neg_args = {"negative"} ;

  e.loadPlatform("../../platforms/cluster.xml");

  simgrid::s4u::ActorPtr positive_actor = simgrid::s4u::Actor::createActor("positive_sampler", simgrid::s4u::Host::by_name("node-0.acme.org"), sampler, pos_args);
  simgrid::s4u::ActorPtr negative_actor = simgrid::s4u::Actor::createActor("negative_sampler", simgrid::s4u::Host::by_name("node-1.acme.org"), sampler, neg_args);
  simgrid::s4u::Actor::createActor("controller", simgrid::s4u::Host::by_name("node-1.acme.org"), controller, pos_args);
  simgrid::s4u::Actor::createActor("cycle_counter", simgrid::s4u::Host::by_name("node-0.acme.org"), cycleCounter, pos_args);

  std::string fmu_uri = "file:///home/mecsyco/Documents/sine_standalone";
  std::string fmu_name = "sine_standalone";
  const double step_size = 0.1;

  std::vector<std::string> initRealInputNames = { "omega" };
  std::vector<double> initRealInputVals = { 0.1 * 3.14159265358979323846 };

  std::vector<std::string> realOutputNames = { "x" };
  std::vector<std::string> boolOutputNames = { "positive" };
  std::vector<std::string> intOutputNames = { "cycles" };

  simgrid::fmi::FMIPlugin::addFMUCS(fmu_uri, fmu_name, step_size,
		  &initRealInputNames, &initRealInputVals,
		  nullptr, nullptr,
		  nullptr, nullptr,
		  nullptr, nullptr,
		  &realOutputNames, &intOutputNames, &boolOutputNames, nullptr);

  std::vector<std::string> params = {std::to_string(negative_actor->getPid()),std::to_string(positive_actor->getPid()), "negative"};
  simgrid::fmi::FMIPlugin::getFMU("sine_standalone")->registerEvent(reactOnNegativeValue, switchActors, params);

  e.run();

  return 0;
}
