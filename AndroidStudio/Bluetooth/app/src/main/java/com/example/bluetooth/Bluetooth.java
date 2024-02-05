package com.example.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.util.Log;
import android.widget.Toast;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.UUID;

public class Bluetooth {

    private static final String TAG = MainActivity.class.getSimpleName();

    private static final UUID BT_MODULE_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); // "random" unique identifier

    public final static int REQUEST_ENABLE_BT = 1; // used to identify adding bluetooth names
    public final static int MESSAGE_READ = 2; // used in bluetooth handler to identify message update
    public final static int CONNECTING_STATUS = 3; // used in bluetooth handler to identify message status
    public final static int BLUETOOTH_NOT_ON = 4;// used in bluetooth handler to identify state
    public final static int CONNECTING_ERROR = 5;
    public final static int READING_ERROR = 6;
    public final static int CONNECTED = 7;
    public final static int NOT_CONNECTED = 8;

    private static BluetoothAdapter mBTAdapter;

    private static BluetoothSocket mBTSocket = null; // bi-directional client-to-client data path
    private static ConnectedThread mConnectedThread; // bluetooth background worker thread to send and receive data
    private static Handler mHandler;

    public static String deviceAddress;
    public static String deviceName;
    public static void SetHandler(Handler handler) {
        mHandler = handler;
        mConnectedThread.SetHandler(handler);
    }

    public static void Connect(Handler handler, String address,String name) {
        mHandler = handler;
        mBTAdapter = BluetoothAdapter.getDefaultAdapter(); // get a handle on the bluetooth radio

        if (!mBTAdapter.isEnabled()) {
            mHandler.obtainMessage(BLUETOOTH_NOT_ON, -1, -1)
                    .sendToTarget();
            return;
        }

        if (mConnectedThread != null)
            mConnectedThread.cancel();

        deviceAddress=address;
        deviceName=name;

        new Thread() {
            @Override
            public void run() {
                boolean fail = false;

                BluetoothDevice device = mBTAdapter.getRemoteDevice(deviceAddress);

                try {
                    mBTSocket = createBluetoothSocket(device);
                } catch (IOException e) {
                    fail = true;
                    mHandler.obtainMessage(CONNECTING_ERROR, 1, -1, "Socket creation failed 1 : " + e.getMessage())
                            .sendToTarget();
                }
                // Establish the Bluetooth socket connection.
                try {
                    mBTSocket.connect();
                } catch (IOException e) {
                    try {
                        fail = true;
                        mBTSocket.close();
                        mHandler.obtainMessage(CONNECTING_STATUS, -1, -1)
                                .sendToTarget();
                    } catch (IOException e2) {
                        //insert code to deal with this
                        mHandler.obtainMessage(CONNECTING_ERROR, 1, -1, "Socket creation failed 2 : " + e.getMessage())
                                .sendToTarget();
                    }
                }
                if (!fail) {
                    mConnectedThread = new ConnectedThread(mBTSocket, mHandler);
                    mConnectedThread.start();

                    mHandler.obtainMessage(CONNECTED, 1, -1, deviceName)
                            .sendToTarget();
                }
            }
        }.start();
    }

    private static BluetoothSocket createBluetoothSocket(BluetoothDevice device) throws IOException {
        try {
            final Method m = device.getClass().getMethod("createInsecureRfcommSocketToServiceRecord", UUID.class);
            return (BluetoothSocket) m.invoke(device, BT_MODULE_UUID);
        } catch (Exception e) {
            Log.e(TAG, "Could not create Insecure RFComm Connection", e);
        }
        return device.createRfcommSocketToServiceRecord(BT_MODULE_UUID);
    }

    public static void Cancel() {
        if (mConnectedThread != null)
            mConnectedThread.cancel();
    }

    public static void SendMessage(String message) {
        if (mConnectedThread != null) {
            if (mConnectedThread.isConnected())
                mConnectedThread.write(message);
            else
                mHandler.obtainMessage(NOT_CONNECTED, 1, -1,"isConnect = False")
                        .sendToTarget();
        }
        else
            mHandler.obtainMessage(NOT_CONNECTED, 1, -1,"mConnectedThread is null")
                    .sendToTarget();
    }

    public static boolean IsConnected()
    {
        return mBTSocket.isConnected();
    }
}
