model lorenz_x

parameter Real sigma = 10;

input Real y;
output Real x;

initial equation
 x = 1;

equation
 der(x) = sigma * (y - x);

end lorenz_x;