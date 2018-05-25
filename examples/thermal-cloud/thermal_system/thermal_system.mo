model thermal_system
  input Real P_load_DC;
  output Real Q_load_DC;
  
  output Real P_other_DC;
  output Real Q_other_DC;
  
  output Real P_chiller;
  output Real Q_cooling;
  
  output Real P_DC;
  output Real Q_DC;
  
  output Real T_R_out;
  Real T_R_in;
    
  input Integer chiller_status;
  output Integer power_supply_status;
  
  parameter Real room_length = 10;
  parameter Real room_width = 6;
  parameter Real room_height = 4;
  parameter Real air_density = 1225;
  parameter Real m_air_total = air_density * room_length * room_width * room_height;
  parameter Real alpha = 0.2;
  parameter Real air_specific_heat = 1.006;
  parameter Real cool_inefficiency = 0.9;
  parameter Real chiller_EER = 0.9;
  parameter Real temp_max = 40;
  parameter Real temp_desired = 24;
  
  initial equation
    power_supply_status = 1;
    T_R_in = temp_desired;
    
  equation
   Q_load_DC = P_load_DC;
   
   P_other_DC = P_load_DC * alpha;
   Q_other_DC = P_other_DC;
   
   P_DC = P_load_DC + P_other_DC + P_chiller;
   Q_DC = Q_load_DC + Q_other_DC;
   
   P_chiller = Q_cooling / chiller_EER;

  
   if chiller_status == 1 then
    T_R_out = T_R_in + Q_DC / (m_air_total * air_specific_heat);
    der(T_R_in) = 0;
    Q_cooling = Q_DC / cool_inefficiency;   
   else
    T_R_out = T_R_in;
    der(T_R_in) = Q_DC / (m_air_total * air_specific_heat);
    Q_cooling = 0;
   end if;
   
   when T_R_in >= temp_max then
    power_supply_status = 0;
   end when;
   
   when chiller_status == 0 then
    reinit(T_R_in, T_R_in + Q_DC / (m_air_total * air_specific_heat));
   end when;
 
  
end thermal_system;
