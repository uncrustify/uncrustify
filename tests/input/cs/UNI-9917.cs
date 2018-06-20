// 1. Formatting the first produces the second.

namespace Unity.IL2CPP.IntegrationTests.ILTests.Tests
{
	public class GlobalsWithBoxOptimizationAndBrTrueOpcodeTest : BoxTestsBase { public GlobalsWithBoxOptimizationAndBrTrueOpcodeTest() : base(OpCodes.Brtrue) { } }
	public class GlobalsWithBoxOptimizationAndBrTrueSOpcodeTest : BoxTestsBase { public GlobalsWithBoxOptimizationAndBrTrueSOpcodeTest() : base(OpCodes.Brtrue_S) { } }
	public class GlobalsWithBoxOptimizationAndBrFalseOpcodeTest : BoxTestsBase { public GlobalsWithBoxOptimizationAndBrFalseOpcodeTest() : base(OpCodes.Brfalse) { } }
	public class GlobalsWithBoxOptimizationAndBrFalseSOpcodeTest : BoxTestsBase { public GlobalsWithBoxOptimizationAndBrFalseSOpcodeTest() : base(OpCodes.Brfalse_S) { } }
}

// 2. And formatting this produces the third.

namespace Unity.IL2CPP.IntegrationTests.ILTests.Tests
{
    public class GlobalsWithBoxOptimizationAndBrTrueOpcodeTest : BoxTestsBase { public GlobalsWithBoxOptimizationAndBrTrueOpcodeTest() : base(OpCodes.Brtrue) {}
    }
    public class GlobalsWithBoxOptimizationAndBrTrueSOpcodeTest : BoxTestsBase { public GlobalsWithBoxOptimizationAndBrTrueSOpcodeTest() : base(OpCodes.Brtrue_S) {}
    }
    public class GlobalsWithBoxOptimizationAndBrFalseOpcodeTest : BoxTestsBase { public GlobalsWithBoxOptimizationAndBrFalseOpcodeTest() : base(OpCodes.Brfalse) {}
    }
    public class GlobalsWithBoxOptimizationAndBrFalseSOpcodeTest : BoxTestsBase { public GlobalsWithBoxOptimizationAndBrFalseSOpcodeTest() : base(OpCodes.Brfalse_S) {}
    }
}

// 3. This doesn't changes when formatted.

namespace Unity.IL2CPP.IntegrationTests.ILTests.Tests
{
    public class GlobalsWithBoxOptimizationAndBrTrueOpcodeTest : BoxTestsBase 
    { 
        public GlobalsWithBoxOptimizationAndBrTrueOpcodeTest() : base(OpCodes.Brtrue) { } 
    }
    public class GlobalsWithBoxOptimizationAndBrTrueSOpcodeTest : BoxTestsBase 
    { 
        public GlobalsWithBoxOptimizationAndBrTrueSOpcodeTest() : base(OpCodes.Brtrue_S) { } 
    }
    public class GlobalsWithBoxOptimizationAndBrFalseOpcodeTest : BoxTestsBase 
    { 
        public GlobalsWithBoxOptimizationAndBrFalseOpcodeTest() : base(OpCodes.Brfalse) { } 
    }
    public class GlobalsWithBoxOptimizationAndBrFalseSOpcodeTest : BoxTestsBase 
    { 
        public GlobalsWithBoxOptimizationAndBrFalseSOpcodeTest() : base(OpCodes.Brfalse_S) { } 
    }
}