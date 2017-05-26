/** 
 * The CountingApp class implements an application that
 * simply prints "X Hello" to standard output for X
 * from 0 to 9, pausing 1 second between each line.
 */
class CountingApp {
    public static void main(String[] args) throws InterruptedException {
        for (int i = 0; i < 10; ++i) {
            System.out.print(i);
            System.out.println(" Hello");
            Thread.sleep(1000);
        }
    }
}
