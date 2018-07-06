model lorenz_z

parameter Real beta = 8/3;

output Real z;
input Real x, y;

initial equation
 z = 1;

equation
 der(z) = x * y - beta * z;

end lorenz_z;