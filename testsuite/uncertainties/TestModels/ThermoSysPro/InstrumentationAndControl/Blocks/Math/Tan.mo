within ThermoSysPro.InstrumentationAndControl.Blocks.Math;
block Tan
  annotation(Icon(coordinateSystem(extent={{-100,-100},{100,100}}), graphics={Text(lineColor={0,0,255}, extent={{-150,150},{150,110}}, textString="%name"),Rectangle(lineColor={0,0,255}, extent={{-100,100},{100,-100}}),Polygon(points={{0,90},{-8,68},{8,68},{0,90}}, fillPattern=FillPattern.Solid, lineColor={192,192,192}, fillColor={192,192,192}),Line(points={{0,-80},{0,68}}, color={192,192,192}),Line(points={{-80,-80},{-78.4,-68.4},{-76.8,-59.7},{-74.4,-50},{-71.2,-40.9},{-67.1,-33},{-60.7,-24.8},{-51.1,-17.2},{-35.8,-9.98},{-4.42,-1.07},{33.4,9.12},{49.4,16.2},{59.1,23.2},{65.5,30.6},{70.4,39.1},{73.6,47.4},{76,56.1},{77.6,63.8},{80,80}}, color={0,0,0}),Line(points={{-90,0},{68,0}}, color={192,192,192}),Polygon(points={{90,0},{68,8},{68,-8},{90,0}}, fillPattern=FillPattern.Solid, lineColor={192,192,192}, fillColor={192,192,192}),Text(lineColor={0,0,255}, extent={{-90,72},{-18,24}}, textString="tan", fillColor={192,192,192})}), Diagram(coordinateSystem(extent={{-100,-100},{100,100}}), graphics={Line(points={{0,80},{-8,80}}, color={192,192,192}),Line(points={{0,-80},{-8,-80}}, color={192,192,192}),Line(points={{0,-88},{0,86}}, color={192,192,192}),Text(lineColor={0,0,255}, extent={{1,104},{28,84}}, textString="y", fillColor={160,160,160}),Polygon(points={{0,102},{-6,86},{6,86},{0,102}}, fillPattern=FillPattern.Solid, lineColor={192,192,192}, fillColor={192,192,192}),Text(lineColor={0,0,255}, extent={{-37,-72},{-17,-88}}, textString="-5.8"),Text(lineColor={0,0,255}, extent={{-33,86},{-13,70}}, textString=" 5.8"),Text(lineColor={0,0,255}, extent={{70,25},{90,5}}, textString="1.4"),Line(points={{-100,0},{84,0}}, color={192,192,192}),Polygon(points={{100,0},{84,6},{84,-6},{100,0}}, fillPattern=FillPattern.Solid, lineColor={192,192,192}, fillColor={192,192,192}),Line(points={{-80,-80},{-78.4,-68.4},{-76.8,-59.7},{-74.4,-50},{-71.2,-40.9},{-67.1,-33},{-60.7,-24.8},{-51.1,-17.2},{-35.8,-9.98},{-4.42,-1.07},{33.4,9.12},{49.4,16.2},{59.1,23.2},{65.5,30.6},{70.4,39.1},{73.6,47.4},{76,56.1},{77.6,63.8},{80,80}}, color={0,0,0}),Text(lineColor={0,0,255}, extent={{70,-6},{94,-26}}, textString="u", fillColor={160,160,160})}), Documentation(info="<html>
<p><b>Adapted from the Modelica.Blocks.Math library</b></p>
</HTML>
<html>
<p><b>Version 1.0</b></p>
</HTML>
"));
  ThermoSysPro.InstrumentationAndControl.Connectors.InputReal u annotation(Placement(transformation(x=-110.0, y=0.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false), iconTransformation(x=-110.0, y=0.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.InstrumentationAndControl.Connectors.OutputReal y annotation(Placement(transformation(x=110.0, y=0.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false), iconTransformation(x=110.0, y=0.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
equation 
  y.signal=Modelica.Math.tan(u.signal);
end Tan;