model chiller_failure
 
  input Real chiller_load;
  output Integer chiller_status;
  parameter Real critic_load = 23000;
  
  initial equation
    chiller_status = 1;
  
  equation
  
    when chiller_load >= critic_load then
      chiller_status = 0;
    end when;
  
end chiller_failure;