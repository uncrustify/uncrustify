public class Foo {private Runnable bar=new Runnable(){
			  @Override
			  @SuppressWarnings("baz") public void run(){
				  quux();
			  }
		  };}
