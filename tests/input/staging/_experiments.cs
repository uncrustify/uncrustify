        [Test]
        public void MethodWithOutAndRefParams()
        {
            AssertHookInjection(
                source,
                "MethodWithOutAndRefParams", (types, args) =>
                {
                    args[0] = 666;
                    args[1] = -1;
                    args[2] = (int)args[2] - 4;

                    return null;
                },
                null,
                originalArgs);
        }

        [Test]
        public void TestPropertySetter()
        {
            bool hasBeenCalled = false;
            AssertHookInjection("class C { public int P { set { } } }", "TestPropertySetter", (_, __) =>
            {
                hasBeenCalled = true;
                return null;
            },
            null);

            Assert.That(hasBeenCalled, Is.True);
        }

                baseType = baseType.MakeGenericInstanceType(
                    method.Parameters.Select<ParameterDefinition, TypeReference>(p => ResolveType(target, typeDefinition, p.ParameterType))
                        .Concat(new[] {ResolveType(target, typeDefinition, method.ReturnType)})
                        .ToArray());


            var realInvoke = new MethodReference(openInvoke.Name, openInvoke.ReturnType,
                openInvoke.DeclaringType.MakeGenericInstanceType(genericType.GenericArguments.ToArray()))


        public static bool IsPublic(this PropertyDefinition property)
        {
            if (property.GetMethod != null && property.GetMethod.IsPublic)
                return true;
            if (property.SetMethod != null && property.SetMethod.IsPublic)
                return true;
            if (property.OtherMethods?.Any(m => m.IsPublic) ?? false)
                return true;

            return false;
        }
