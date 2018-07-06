model lorenz_y

parameter Real rho = 28;

output Real y;
input Real x, z;

initial equation
 y = 1;

equation
 der(y) = x * (rho - z) - y;

end lorenz_y;