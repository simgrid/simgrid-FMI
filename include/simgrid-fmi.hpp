#ifndef INCLUDE_SIMGRID_PLUGINS_FMI_HPP_
#define INCLUDE_SIMGRID_PLUGINS_FMI_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include <simgrid/kernel/resource/Model.hpp>
#include "FMUCoSimulation_v1.h"
#include "FMUCoSimulation_v2.h"
#include "ModelManager.h"
#include <boost/functional/hash.hpp>

struct port{
	std::string name;
	std::string fmu;
};

bool operator== (port a, port b){
	return (a.fmu == b.fmu) && (a.name == b.name);
}

namespace std{
	using boost::hash_value;
	using boost::hash_combine;
	template<> struct hash<port>{
		size_t operator()(const port& k) const{
			std::size_t seed = 0;
			hash_combine(seed,hash_value(k.fmu));
			hash_combine(seed,hash_value(k.name));

			return seed;
		}
	};
}

namespace simgrid{
namespace fmi{

struct real_simgrid_fmu_connection{
	port in;
	double (*generateInput)(std::vector<std::string>);
	std::vector<std::string> params;
};

struct integer_simgrid_fmu_connection{
	port in;
	int (*generateInput)(std::vector<std::string>);
	std::vector<std::string> params;
};

struct boolean_simgrid_fmu_connection{
	port in;
	bool (*generateInput)(std::vector<std::string>);
	std::vector<std::string> params;
};

struct string_simgrid_fmu_connection{
	port in;
	std::string (*generateInput)(std::vector<std::string>);
	std::vector<std::string> params;
};


class MasterFMI : public simgrid::kernel::resource::Model{

private:
	/*
	 * The set of FMUs to simulate
	 */
	std::unordered_map<std::string,FMUCoSimulationBase*> fmus;
	/*
	 * Indicate if an FMU require an iteration (i.e. doStep(O)) to update its outputs when setting input
	 */
	std::unordered_map<std::string,bool> iterate_input;

	/**
	 * coupling between FMUs (nb: key=input value=output !)
	 */
	std::unordered_map<port,port> couplings;
	std::vector<port> in_coupled_input;

	/**
	 * coupling between SimGrid models and FMUs
	 */
	std::vector<real_simgrid_fmu_connection> real_ext_couplings;
	std::vector<integer_simgrid_fmu_connection> integer_ext_couplings;
	std::vector<boolean_simgrid_fmu_connection> boolean_ext_couplings;
	std::vector<string_simgrid_fmu_connection> string_ext_couplings;

	std::vector<port> ext_coupled_input;


	/**
	 * last output values send to the input
	 */
	std::unordered_map<std::string,double> last_real_outputs;
	std::unordered_map<std::string,int> last_int_outputs;
	std::unordered_map<std::string,bool> last_bool_outputs;
	std::unordered_map<std::string,std::string> last_string_outputs;

	double nextEvent;
	double commStep;
	double current_time;

	bool firstEvent;

	std::vector<void (*)(std::vector<std::string>)> event_handlers;
	std::vector<bool (*)(std::vector<std::string>)> event_conditions;
	std::vector<std::vector<std::string>> event_params;

	void manageEventNotification();
	void solveCouplings(bool firstIteration);
	bool solveCoupling(port in, port out, bool checkChange);
	void solveExternalCoupling();

	bool isInputCoupled(std::string fmu, std::string input_name);

public:
	MasterFMI(const double stepSize);
	~MasterFMI();
	void addFMUCS(std::string fmu_uri, std::string fmu_name, bool iterateAfterInput);
	void update_actions_state(double now, double delta) override;
	double getRealOutput(std::string fmi_name, std::string output_name);
	bool getBooleanOutput(std::string fmi_name, std::string output_name);
	int getIntegerOutput(std::string fmi_name, std::string output_name);
	std::string getStringOutput(std::string fmi_name, std::string output_name);
	void setRealInput(std::string fmi_name, std::string input_name, double value, bool simgrid_input);
	void setBooleanInput(std::string fmi_name, std::string input_name, bool value, bool simgrid_input);
	void setIntegerInput(std::string fmi_name, std::string input_name, int value, bool simgrid_input);
	void setStringInput(std::string fmi_name, std::string input_name, std::string value, bool simgrid_input);
	double next_occuring_event(double now) override;
	void registerEvent(bool (*condition)(std::vector<std::string>), void (*handleEvent)(std::vector<std::string>), std::vector<std::string> params);
	void deleteEvents();
	void connectFMU(std::string out_fmu_name,std::string output_port,std::string in_fmu_name,std::string input_port);
	void connectRealFMUToSimgrid(double (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name);
	void connectIntegerFMUToSimgrid(int (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name);
	void connectBooleanFMUToSimgrid(bool (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name);
	void connectStringFMUToSimgrid(std::string (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name);
	void initCouplings();

};


class FMIPlugin {

public:
	static void addFMUCS(std::string fmu_uri, std::string fmu_name, bool iterateAfterInput=true);
	static void connectFMU(std::string out_fmu_name,std::string output_port,std::string in_fmu_name,std::string input_port);
	static void initFMIPlugin(double communication_step);
	static double getRealOutput(std::string fmi_name, std::string output_name);
	static bool getBooleanOutput(std::string fmi_name, std::string output_name);
	static int getIntegerOutput(std::string fmi_name, std::string output_name);
	static std::string getStringOutput(std::string fmi_name, std::string output_name);
	static void setRealInput(std::string fmi_name, std::string input_name, double value);
	static void setBooleanInput(std::string fmi_name, std::string input_name, bool value);
	static void setIntegerInput(std::string fmi_name, std::string input_name, int value);
	static void setStringInput(std::string fmi_name, std::string input_name, std::string value);
	static void registerEvent(bool (*condition)(std::vector<std::string>), void (*handleEvent)(std::vector<std::string>), std::vector<std::string> params);
	static void deleteEvents();
	static void connectRealFMUToSimgrid(double (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name);
	static void connectIntegerFMUToSimgrid(int (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name);
	static void connectBooleanFMUToSimgrid(bool (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name);
	static void connectStringFMUToSimgrid(std::string (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name);
	static void readyForSimulation();
private:
	FMIPlugin();
	~FMIPlugin();
	static MasterFMI *master;
};

}
}

#endif /* INCLUDE_SIMGRID_PLUGINS_FMI_HPP_ */
