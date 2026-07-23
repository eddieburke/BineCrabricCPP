package net.minecraft.test;

import java.io.IOException;
import java.util.List;

public class JavaParityTestRunner {
    private static int totalPassed = 0;
    private static int totalFailed = 0;

    public static void main(String[] args) {
        String host = args.length > 0 ? args[0] : "127.0.0.1";
        int port = args.length > 1 ? Integer.parseInt(args[1]) : 25565;

        System.out.println("=== Starting Java Multiplayer Parity Test Suite ===");
        System.out.println("Connecting to server at " + host + ":" + port);

        runTest("FullMultiplayerProtocolParitySuite", () -> testFullMultiplayerProtocolParity(host, port));

        System.out.println("\n=== Java Parity Test Suite Summary ===");
        System.out.println("PASSED: " + totalPassed + ", FAILED: " + totalFailed);

        if (totalFailed > 0) {
            System.exit(1);
        }
    }

    private static void runTest(String name, TestRunnable test) {
        System.out.print("[TEST] " + name + " ... ");
        try {
            test.run();
            System.out.println("PASSED");
            totalPassed++;
        } catch (Throwable e) {
            System.out.println("FAILED: " + (e.getMessage() != null ? e.getMessage() : e.getClass().getName()));
            e.printStackTrace();
            totalFailed++;
        }
    }

    private static void testFullMultiplayerProtocolParity(String host, int port) throws Exception {
        try (TestClientSocket client = new TestClientSocket(host, port)) {
            System.out.println("\n  [Stage 1] Login Handshake...");
            client.sendLogin("JavaParityBot");
            int firstPacket = client.readPacket();
            assertEqual(1, firstPacket, "Expected LoginHelloPacket (0x01)");
            assertGreaterThan(-1, client.getPlayerEntityId(), "Expected valid player entity ID");

            boolean receivedSpawn = false;
            boolean chunkStatusReceived = false;
            boolean chunkDataReceived = false;
            boolean moveReceived = false;

            for (int i = 0; i < 60; i++) {
                int pid = client.readPacket();
                if (pid == 6) receivedSpawn = true;
                if (pid == 50) chunkStatusReceived = true;
                if (pid == 51) chunkDataReceived = true;
                if (pid == 13) moveReceived = true;
            }

            assertTrue(receivedSpawn, "Expected PlayerSpawnPositionS2CPacket (0x06)");
            assertTrue(chunkStatusReceived, "Expected ChunkStatusUpdateS2CPacket (0x32)");

            System.out.println("  [Stage 2] Movement & Stance Sync...");
            double feetY = 64.0;
            double stance = feetY + 1.62;
            client.sendPlayerMove(8.5, feetY, stance, 8.5, 0.0f, 0.0f, true);

            for (int i = 0; i < 20; i++) {
                int pid = client.readPacket();
                if (pid == 13) {
                    double rFeetY = client.getLastReceivedFeetY();
                    double rStance = client.getLastReceivedStance();
                    double stanceDelta = rStance - rFeetY;
                    assertClose(1.62, stanceDelta, 0.05, "Expected stance delta near 1.62");
                    break;
                }
            }

            System.out.println("  [Stage 3] Block Action & Interactivity...");
            client.sendPlayerAction(0, 10, 64, 10, 1);
            client.sendPlayerAction(2, 10, 64, 10, 1);

            for (int i = 0; i < 10; i++) {
                client.readPacket();
            }

            List<Integer> pids = client.getReceivedPacketIds();
            assertTrue(pids.size() > 0, "Expected active packet stream");
        }
    }

    private static void assertTrue(boolean condition, String message) {
        if (!condition) {
            throw new AssertionError(message);
        }
    }

    private static void assertEqual(int expected, int actual, String message) {
        if (expected != actual) {
            throw new AssertionError(message + " [Expected: " + expected + ", Got: " + actual + "]");
        }
    }

    private static void assertGreaterThan(int threshold, int actual, String message) {
        if (actual <= threshold) {
            throw new AssertionError(message + " [Expected > " + threshold + ", Got: " + actual + "]");
        }
    }

    private static void assertClose(double expected, double actual, double delta, String message) {
        if (Math.abs(expected - actual) > delta) {
            throw new AssertionError(message + " [Expected: " + expected + " (+/- " + delta + "), Got: " + actual + "]");
        }
    }

    @FunctionalInterface
    interface TestRunnable {
        void run() throws Exception;
    }
}
