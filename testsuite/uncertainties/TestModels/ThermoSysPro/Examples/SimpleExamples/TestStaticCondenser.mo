within ThermoSysPro.Examples.SimpleExamples;
model TestStaticCondenser
  annotation(Diagram, Diagram(coordinateSystem(extent={{-200,-200},{200,200}})), Icon(coordinateSystem(extent={{-200,-200},{200,200}})));
  WaterSteam.BoundaryConditions.SourceQ Source_condenseur(h0=60000.0, Q0(fixed=true)=4000) annotation(Placement(transformation(x=-170.0, y=10.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  WaterSteam.BoundaryConditions.SinkP Puit_condenseur annotation(Placement(transformation(x=170.0, y=10.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.WaterSteam.HeatExchangers.StaticCondenser condenseur(CPCE=0, KCO=100, QC0=200, z=5, SCO=15000.0, Qee(start=4000, fixed=false), Pcond(fixed=false, start=2154.77)) annotation(Placement(transformation(x=26.0, y=33.0, scale=0.46, aspectRatio=1.06521739130435, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.WaterSteam.BoundaryConditions.SourceP sourceP(P0(fixed=false)=100000.0, option_temperature=2, mode=0, h0=2581700.0, C(Q(fixed=true, start=100))) annotation(Placement(transformation(x=-90.0, y=150.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.WaterSteam.BoundaryConditions.SourceP sourceP1(option_temperature=2, mode=0, h0=2548100.0, C(Q(fixed=true, start=1e-05)), P0(fixed=false)=100000.0) annotation(Placement(transformation(x=-172.0, y=90.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.WaterSteam.BoundaryConditions.SourceP sourceP2(option_temperature=2, mode=0, h0=2505500.0, C(Q(fixed=true, start=1e-05)), P0(fixed=false)=44000) annotation(Placement(transformation(x=-170.0, y=50.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.WaterSteam.PressureLosses.SingularPressureLoss singularPressureLoss(K=10) annotation(Placement(transformation(x=-90.0, y=90.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.WaterSteam.PressureLosses.SingularPressureLoss singularPressureLoss1(K=10) annotation(Placement(transformation(x=-90.0, y=50.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.WaterSteam.PressureLosses.SingularPressureLoss singularPressureLoss2(K=0.0001) annotation(Placement(transformation(x=-90.0, y=10.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.WaterSteam.PressureLosses.SingularPressureLoss singularPressureLoss3(K=10) annotation(Placement(transformation(x=-30.0, y=150.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.WaterSteam.BoundaryConditions.Sink Puit_condenseur1 annotation(Placement(transformation(x=110.0, y=-90.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.WaterSteam.PressureLosses.SingularPressureLoss singularPressureLoss4(K=0.0001) annotation(Placement(transformation(x=110.0, y=10.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
  ThermoSysPro.WaterSteam.PressureLosses.SingularPressureLoss singularPressureLoss5(K=0.0001) annotation(Placement(transformation(x=50.0, y=-90.0, scale=0.1, aspectRatio=1.0, flipHorizontal=false, flipVertical=false)));
equation 
  connect(sourceP1.C,singularPressureLoss.C1) annotation(Line(points={{-162,90},{-100,90}}, color={0,0,255}));
  connect(singularPressureLoss.C2,condenseur.Cev) annotation(Line(points={{-80,90},{-50,90},{-50,62.89},{-20,62.89}}, color={0,0,255}));
  connect(sourceP2.C,singularPressureLoss1.C1) annotation(Line(points={{-160,50},{-100,50}}, color={0,0,255}));
  connect(singularPressureLoss1.C2,condenseur.Cep) annotation(Line(points={{-80,50},{-52,50},{-52,42.31},{-20,42.31}}, color={0,0,255}));
  connect(Source_condenseur.C,singularPressureLoss2.C1) annotation(Line(points={{-160,10},{-100,10}}, color={0,0,255}));
  connect(singularPressureLoss2.C2,condenseur.Cee) annotation(Line(points={{-80,10},{-52,10},{-52,3.11},{-20,3.11}}, color={0,0,255}));
  connect(sourceP.C,singularPressureLoss3.C1) annotation(Line(points={{-80,150},{-40,150}}, color={0,0,255}));
  connect(singularPressureLoss3.C2,condenseur.Cvt) annotation(Line(points={{-20,150},{26,150},{26,82.49}}, color={0,0,255}));
  connect(singularPressureLoss4.C2,Puit_condenseur.C) annotation(Line(points={{120,10},{160,10}}, color={0,0,255}));
  connect(condenseur.Cse,singularPressureLoss4.C1) annotation(Line(points={{72.92,3.11},{86,3.11},{86,10},{100,10}}, color={0,0,255}));
  connect(singularPressureLoss5.C2,Puit_condenseur1.C) annotation(Line(points={{60,-90},{100,-90}}, color={0,0,255}));
  connect(singularPressureLoss5.C1,condenseur.Cex) annotation(Line(points={{40,-90},{26.46,-90},{26.46,-16.98}}));
end TestStaticCondenser;
