package com.example.bluetooth;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.preference.PreferenceManager;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.io.UnsupportedEncodingException;

public class SplashScreen extends AppCompatActivity {

    String key;
    String deviceMac;
    String deviceName;
    SharedPreferences preferences;
    SharedPreferences.Editor editor;
    private static Handler mHandler;
    TextView status;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_splash_screen);

        status=(TextView)findViewById(R.id.status);
        preferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        editor = preferences.edit();

        deviceMac=preferences.getString("mac","");
        deviceName=preferences.getString("name","");
        key=preferences.getString("key","");

        if(deviceMac=="" || deviceName=="") {
            GotoConnect();
            return;
        }
        else
            status.setText(deviceName.replace("\n","") + " Bağlanıyor");

        mHandler = new Handler(Looper.getMainLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case Bluetooth.MESSAGE_READ: {
                        String readMessage = null;
                        try {
                            readMessage = new String((byte[]) msg.obj, "UTF-8");
                            if (readMessage.indexOf("OK") == 0) {
                                Toast.makeText(getBaseContext(), "Bağlandı : " +  deviceName, Toast.LENGTH_SHORT).show();
                                GotoDeshboard();
                            }
                            else
                            {
                                Bluetooth.Cancel();
                                Toast.makeText(getBaseContext(), "Cihaz ile eşleşmiyor : " +  deviceName, Toast.LENGTH_SHORT).show();
                                GotoConnect();
                            }

                        } catch (UnsupportedEncodingException e) {
                            e.printStackTrace();
                            GotoConnect();
                        }
                        break;
                    }
                    case Bluetooth.CONNECTED: {
                        Bluetooth.SendMessage("(#" + key + "#;0)");
                        break;
                    }
                }
            }
        };

        Bluetooth.Connect(mHandler, deviceMac, deviceName);

        mHandler.removeCallbacks(mUpdateTimeTask);
        mHandler.postDelayed(mUpdateTimeTask, 5000);
    }

    private Runnable mUpdateTimeTask = new Runnable() {
        public void run() {
            //Toast.makeText(getBaseContext(), "Timer Çalıştı " , Toast.LENGTH_SHORT).show();
            GotoConnect();
            // buraya ne yapmak istiyorsan o kodu yaz.. Kodun sonlandıktan sonra 1 saniye sonra tekrar çalışacak şekilde handler tekrar çalışacak.
            //mHandler.postDelayed(this, 1000);
        }
    };

    private void GotoConnect()
    {
        if(mHandler!=null)
            mHandler.removeCallbacks(mUpdateTimeTask);
        Intent i = new Intent(getApplicationContext(),Connect.class);
        startActivity(i);
        finish();
    }

    private void GotoDeshboard()
    {
        if(mHandler!=null)
            mHandler.removeCallbacks(mUpdateTimeTask);
        Intent i = new Intent(getApplicationContext(),Deshboard.class);
        startActivity(i);
        finish();
    }
}