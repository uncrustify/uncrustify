public class Foo {

    public static void main(String[] args) {
        for (ProcessDefinition processDefinition:
             allOfTheDefinitions.getData()) {
            doit(processDefinition);
        }
    }
}
