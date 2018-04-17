import java.util.Objects;
import java.util.function.Predicate;

public class Java8DoubleColon {
public static void main(String[] args) {
	Predicate<Object> p = Objects::nonNull;
	System.out.println(false == p.test(null));
	System.out.println(true == p.test(p));
}
}