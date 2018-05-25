#include "simgrid/s4u.hpp"
#include "simgrid/plugins/fmi.hpp"
#include <IntegratorType.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(main, "Messages specific for this msg example");

/**
 * Event detection functions
 */
static bool reactOnRealValueChange(std::vector<std::string> args){
	int current_value = std::stoi(args[0],0,10);
	return simgrid::fmi::FMIPlugin::getFMU("zigzag")->getRealOutputs(args[2]) != current_value;

}

static bool reactOnIntegerValueChange(std::vector<std::string> args){
	int current_value = std::stoi(args[0],0,10);
	return simgrid::fmi::FMIPlugin::getFMU("zigzag")->getIntegerOutputs(args[2]) != current_value;

}

/**
 * Event handlers
 */

static void wakeUpActor(std::vector<std::string> args){
	unsigned long pid_on = std::stoul(args[1],0,10);
	simgrid::s4u::Actor::byPid(pid_on)->resume();
}


/**
 * Actor behaviors
 */

static int sampler(std::vector<std::string> args)
{

  double time = simgrid::s4u::Engine::getClock();
  double time_step = 0.15;

  while(time < 5.0 ){

	  double x = simgrid::fmi::FMIPlugin::getFMU("zigzag")->getRealOutputs("x");
	  double derx = simgrid::fmi::FMIPlugin::getFMU("zigzag")->getRealOutputs("derx");
	  double out = simgrid::fmi::FMIPlugin::getFMU("zigzag")->getIntegerOutputs("out");

	  XBT_INFO("x = %f , derx = %f, out = %f",x,derx, out);

	  simgrid::s4u::this_actor::sleep_for(time_step);

	  time = simgrid::s4u::Engine::getClock();
  }

  simgrid::fmi::FMIPlugin::getFMU("zigzag")->deleteEvents();
  exit(0);
  return 0;
}


static int thresholdNotifier(std::vector<std::string> args){

	unsigned long pid = simgrid::s4u::Actor::self()->getPid();
	double derx = simgrid::fmi::FMIPlugin::getFMU("zigzag")->getRealOutputs("derx");

	while(1){

		std::vector<std::string> params = {std::to_string(derx), std::to_string(pid), "derx"};

		simgrid::fmi::FMIPlugin::getFMU("zigzag")->registerEvent(reactOnRealValueChange,wakeUpActor,params);
		simgrid::s4u::Actor::self()->suspend();
		double x = simgrid::fmi::FMIPlugin::getFMU("zigzag")->getRealOutputs("x");
		derx = simgrid::fmi::FMIPlugin::getFMU("zigzag")->getRealOutputs("derx");
		double out = simgrid::fmi::FMIPlugin::getFMU("zigzag")->getIntegerOutputs("out");

		XBT_INFO("Threshold reached !!! x = %f , derx = %f, out = %f",x,derx, out);
	}

	return 0;
}

static int timeEventNotifier(std::vector<std::string> args){

	unsigned long pid = simgrid::s4u::Actor::self()->getPid();
	double out = simgrid::fmi::FMIPlugin::getFMU("zigzag")->getIntegerOutputs("out");

	while(1){

		std::vector<std::string> params = {std::to_string(out), std::to_string(pid), "out"};

		simgrid::fmi::FMIPlugin::getFMU("zigzag")->registerEvent(reactOnIntegerValueChange,wakeUpActor,params);
		simgrid::s4u::Actor::self()->suspend();
		double x = simgrid::fmi::FMIPlugin::getFMU("zigzag")->getRealOutputs("x");
		double derx = simgrid::fmi::FMIPlugin::getFMU("zigzag")->getRealOutputs("derx");
		out = simgrid::fmi::FMIPlugin::getFMU("zigzag")->getIntegerOutputs("out");

		XBT_INFO("Time-event reached !!! x = %f , derx = %f, out = %f",x,derx, out);
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

  e.loadPlatform("../../platforms/cluster.xml");

  std::vector<std::string> args = {"positive"} ;


  simgrid::s4u::ActorPtr positive_actor = simgrid::s4u::Actor::createActor("positive_sampler", simgrid::s4u::Host::by_name("node-0.acme.org"), sampler, args);
  simgrid::s4u::ActorPtr threshold_actor = simgrid::s4u::Actor::createActor("threshold_notifier", simgrid::s4u::Host::by_name("node-1.acme.org"), thresholdNotifier, args);
  simgrid::s4u::ActorPtr state_event_actor = simgrid::s4u::Actor::createActor("time_events_notifier", simgrid::s4u::Host::by_name("node-1.acme.org"), timeEventNotifier, args);


  std::string fmu_uri = "file:///home/mecsyco/Documents/zigzag";
  std::string fmu_name = "zigzag";

  const double stepsize = 0.1;
  const double lookahead = 0.2;
  const double intstepsize = 0.01;

  std::vector<std::string> initRealInputNames = { "d" };
  std::vector<double> initRealInputVals = { 10.0 };

  std::vector<std::string> realOutputNames = { "x" , "derx"};
  std::vector<std::string> intOutputNames = { "out" };


  simgrid::fmi::FMIPlugin::addFMUME(fmu_uri, fmu_name, intstepsize, stepsize, lookahead,
		  IntegratorType::dp,
		  &initRealInputNames, &initRealInputVals,
		  nullptr, nullptr,
		  nullptr, nullptr,
		  nullptr, nullptr,
		  &realOutputNames, &intOutputNames, nullptr, nullptr);

  e.run();

  return 0;
}
