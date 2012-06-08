within ThermoSysPro.Properties.FlueGases;
function FlueGases_h "Specific enthalpy"
  input ThermoSysPro.Units.AbsolutePressure PMF "Flue gases average pressure";
  input ThermoSysPro.Units.AbsoluteTemperature TMF "Flue gases average temperature";
  input Real Xco2 "CO2 mass fraction";
  input Real Xh2o "H2O mass fraction";
  input Real Xo2 "O2 mass fraction";
  input Real Xso2 "SO2 mass fraction";
  output ThermoSysPro.Units.SpecificEnthalpy h "Specific enthalpy";
protected 
  ThermoSysPro.Properties.ModelicaMediaFlueGases.ThermodynamicState state;
  ThermoSysPro.Properties.ModelicaMediaFlueGases.ThermodynamicState state0;
  Real Xn2 "N2 mass fraction";
  constant Real Hlat=2501599.9019 "Phase transition energy";
algorithm 
  Xn2:=1 - Xco2 - Xh2o - Xo2 - Xso2;
  state.p:=PMF;
  state.T:=TMF;
  state.X:={Xn2,Xo2,Xh2o,Xco2,Xso2};
  state0.p:=0.006112*100000.0;
  state0.T:=273.16;
  state0.X:={Xn2,Xo2,Xh2o,Xco2,Xso2};
  h:=ThermoSysPro.Properties.ModelicaMediaFlueGases.specificEnthalpy(state) - ThermoSysPro.Properties.ModelicaMediaFlueGases.specificEnthalpy(state0) + Xh2o*Hlat;
  annotation(smoothOrder=2, Icon(coordinateSystem(extent={{-100,-100},{100,100}}), graphics={Text(lineColor={0,0,255}, extent={{-136,102},{140,42}}, textString="%name"),Ellipse(extent={{-100,40},{100,-100}}, fillPattern=FillPattern.Solid, lineColor={255,127,0}, fillColor={255,255,255}),Text(lineColor={0,0,255}, extent={{-84,-4},{84,-52}}, textString="function", fillColor={255,127,0})}), Documentation(info="<html>
<p><b>Copyright &copy; EDF 2002 - 2010</b></p>
</HTML>
<html>
<p><b>ThermoSysPro Version 2.0</b></p>
</HTML>
"), extent=[-80,60;-60,80]);
end FlueGases_h;
