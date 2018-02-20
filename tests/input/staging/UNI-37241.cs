public double TotalPurchases { get; set; }
protected IEnumerable<string> Defines { get; } =
TargetPlatformRules.Elements
	.Append("LINUX")
	.Append("_RAKNET_LIB");