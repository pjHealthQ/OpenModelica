within ThermoSysPro.InstrumentationAndControl.Blocks.Math;
block Log
  annotation(Icon(coordinateSystem(extent={{-100,-100},{100,100}}), graphics={Text(lineColor={0,0,255}, extent={{-150,150},{150,110}}, textString="%name"),Rectangle(lineColor={0,0,255}, extent={{-100,100},{100,-100}}),Line(points={{-80,-80},{-80,68}}, color={192,192,192}),Polygon(points={{-80,90},{-88,68},{-72,68},{-80,90}}, fillPattern=FillPattern.Solid, lineColor={192,192,192}, fillColor={192,192,192}),Line(points={{-80,-80},{-79.2,-50.6},{-78.4,-37},{-77.6,-28},{-76.8,-21.3},{-75.2,-11.4},{-72.8,-1.31},{-69.5,8.08},{-64.7,17.9},{-57.5,28},{-47,38.1},{-31.8,48.1},{-10.1,58},{22.1,68},{68.7,78.1},{80,80}}, color={0,0,0}),Line(points={{-90,0},{68,0}}, color={192,192,192}),Polygon(points={{90,0},{68,8},{68,-8},{90,0}}, fillPattern=FillPattern.Solid, lineColor={192,192,192}, fillColor={192,192,192}),Text(lineColor={0,0,255}, extent={{-6,-24},{66,-72}}, textString="log", fillColor={192,192,192})}), Diagram(coordinateSystem(extent={{-100,-100},{100,100}}), graphics={Line(points={{-80,80},{-88,80}}, color={192,192,192}),Line(points={{-80,-80},{-88,-80}}, color={192,192,192}),Line(points={{-80,-90},{-80,84}}, color={192,192,192}),Text(lineColor={0,0,255}, extent={{-75,98},{-48,80}}, textString="y", fillColor={160,160,160}),Polygon(points={{-80,100},{-86,84},{-74,84},{-80,100}}, fillPattern=FillPattern.Solid, lineColor={192,192,192}, fillColor={192,192,192}),Line(points={{-100,0},{84,0}}, color={192,192,192}),Polygon(points={{100,0},{84,6},{84,-6},{100,0}}, fillPattern=FillPattern.Solid, lineColor={192,192,192}, fillColor={192,192,192}),Line(points={{-80,-80},{-79.2,-50.6},{-78.4,-37},{-77.6,-28},{-76.8,-21.3},{-75.2,-11.4},{-72.8,-1.31},{-69.5,8.08},{-64.7,17.9},{-57.5,28},{-47,38.1},{-31.8,48.1},{-10.1,58},{22.1,68},{68.7,78.1},{80,80}}, color={0,0,0}),Text(lineColor={0,0,255}, extent={{-105,72},{-85,88}}, textString="3"),Text(lineColor={0,0,255}, extent={{-109,-88},{-89,-72}}, textString="-3"),Text(lineColor={0,0,255}, extent={{70,-3},{90,-23}}, textString="20"),Text(lineColor={0,0,255}, extent={{-78,-1},{-58,-21}}, textString="1"),Text(lineColor={0,0,255}, extent={{68,28},{94,8}}, textString="u", fillColor={160,160,160})}), Documentation(info="<html>
<p><b>Adapted from the Modelica.Blocks.Math library</b></p>
</HTML>
<html>
<p><b>Version 1.0</b></p>
</HTML>
"));
  ThermoSysPro.InstrumentationAndControl.Connectors.InputReal u annotation(Placement(transformation(x=-110.0, y=0.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false), iconTransformation(x=-110.0, y=0.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.InstrumentationAndControl.Connectors.OutputReal y annotation(Placement(transformation(x=110.0, y=0.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false), iconTransformation(x=110.0, y=0.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
equation 
  y.signal=Modelica.Math.log(u.signal);
end Log;
