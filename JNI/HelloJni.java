
public class HelloJni {

    static{
        System.loadLibrary("hello");
    }

    public native void printHelloWolrd();

    public static void main(String[] args) {
        // TODO Auto-generated method stub

        new HelloJni().printHelloWolrd();                   
    }
}
