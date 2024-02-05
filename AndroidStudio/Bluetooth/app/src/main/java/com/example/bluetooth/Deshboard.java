package com.example.bluetooth;

import androidx.appcompat.app.AppCompatActivity;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.google.gson.Gson;

import java.io.IOException;
import java.io.UnsupportedEncodingException;

import pl.droidsonroids.gif.GifDrawable;
import pl.droidsonroids.gif.GifImageView;

public class Deshboard extends AppCompatActivity {

    //Commands
    private final String GET_VALUES = "1";
    private final String SET_VALUES = "2";
    //
    private final String HEATER_TMP="0";
    private final String FAN="2";
    private final String FAN_SPEED = "3";
    private final String FAN_STOP_TMP = "4";
    private final String FAN_TOLERANCE_TMP = "5";
    private final String POMPA_TMP = "6";

    GifImageView gFunRunImage;
    GifImageView gHeaterImage;
    Button bFanStartBtn;
    Button bFanStopBtn;
    SeekBar sbFanSpeed;
    Spinner sFanStopTmp;
    Spinner sFanToleranceTmp;
    Spinner sPompaTmp;
    Handler mHandler;
    TextView tvFanSpeedValue;
    TextView tvHeaterTmp;
    int heaterTmp=0;

    SharedPreferences preferences;
    SharedPreferences.Editor editor;

    String key;
    boolean isBlock=true;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        preferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        key=preferences.getString("key","");

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_deshboard);
        gFunRunImage=(GifImageView)findViewById(R.id.fan_run_image);
        gHeaterImage=(GifImageView)findViewById(R.id.heater_image);
        bFanStartBtn=(Button)findViewById(R.id.btn_fan_start);
        bFanStopBtn=(Button)findViewById(R.id.btn_fan_stop);
        sbFanSpeed=(SeekBar)findViewById(R.id.fan_speed);
        sFanStopTmp=(Spinner)findViewById(R.id.fan_stop_tmp);
        sFanToleranceTmp=(Spinner)findViewById(R.id.fan_tolerance_tmp);
        sPompaTmp=(Spinner)findViewById(R.id.pompa_tmp);
        tvFanSpeedValue=(TextView)findViewById(R.id.fan_speed_value);
        tvHeaterTmp=(TextView)findViewById(R.id.heater_tmp) ;

        String[] items = new String[41];
        for (int i=40 ; i<=80;i++)
        {
            items[i-40]=String.valueOf(i);
        }

        ArrayAdapter<String> adapter1 = new ArrayAdapter<>(this, R.layout.template_spinner, items);
        sFanStopTmp.setAdapter(adapter1);

        items = new String[16];
        for (int i=5 ; i<=20;i++)
        {
            items[i-5]=String.valueOf(i);
        }

        ArrayAdapter<String> adapter2 = new ArrayAdapter<>(this, R.layout.template_spinner, items);
        sFanToleranceTmp.setAdapter(adapter2);

        items = new String[41];
        for (int i=10 ; i<=50;i++)
        {
            items[i-10]=String.valueOf(i);
        }

        ArrayAdapter<String> adapter3 = new ArrayAdapter<>(this, R.layout.template_spinner, items);
        sPompaTmp.setAdapter(adapter3);

        sbFanSpeed.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
                tvFanSpeedValue.setText(String.valueOf(seekBar.getProgress()));
                FanSpeedChange();
                if(!hasRequest)
                    SetValue(FAN_SPEED,String.valueOf(seekBar.getProgress()),"Fan Speed");
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress,boolean fromUser) {
                // TODO Auto-generated method stub
                //fanSpeedValue.setText(String.valueOf(progress));
            }
        });

        sFanStopTmp.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                if (isBlock) return;
                String value = adapterView.getItemAtPosition(i).toString();
                if (Integer.parseInt(value) != 0) {
                    if (!hasRequest)
                        SetValue(FAN_STOP_TMP,value,"Fan Stop Tmp");
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });

        sFanToleranceTmp.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                if (isBlock) return;
                String value = adapterView.getItemAtPosition(i).toString();
                if (Integer.parseInt(value) != 0) {
                    if (!hasRequest)
                        SetValue(FAN_TOLERANCE_TMP,value,"Fan Tolerance Tmp");
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });

        sPompaTmp.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                if (isBlock) return;
                String value = adapterView.getItemAtPosition(i).toString();
                if (Integer.parseInt(value) != 0) {
                    if (!hasRequest)
                        SetValue(POMPA_TMP,value,"Pompa Tmp");
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {

            }
        });

        bFanStartBtn.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v){
                FanChange(true);
            }
        });

        bFanStopBtn.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v){
                FanChange(false);
            }
        });

        mHandler = new Handler(Looper.getMainLooper()) {
            @Override
            public void handleMessage(Message msg) {
                if (msg.what == Bluetooth.MESSAGE_READ) {
                    String readMessage = null;
                    try {
                        readMessage = new String((byte[]) msg.obj, "UTF-8");
                        Log.i("Gelen Veri :",readMessage);
                        Gson gson = new Gson();
                        Commands cmds = gson.fromJson(readMessage, Commands.class);
                        for (Command cmd : cmds.s) {
                            PrepareObject(cmd);
                        }
                    } catch (UnsupportedEncodingException e) {
                        e.printStackTrace();
                    } catch (Exception e)
                    {
                        Log.e("Exception : " ,e.getMessage());
                    }
                }
            }
        };

        Bluetooth.SetHandler(mHandler);

        mHandler.removeCallbacks(mUnblockTimeTask);
        mHandler.postDelayed(mUnblockTimeTask, 1000);

        IntentFilter filter1 = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
        registerReceiver(mBroadcastReceiver1, filter1);

        IntentFilter filter3 = new IntentFilter();
        filter3.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);
        filter3.addAction(BluetoothDevice.ACTION_ACL_DISCONNECTED);
        registerReceiver(mBroadcastReceiver3, filter3);

        String code = "(#" + key + "#;" + GET_VALUES + ")";
        Log.i("Action Load Get  : ", code);
        Bluetooth.SendMessage(code);
    }

    private Runnable mUnblockTimeTask = new Runnable() {
        public void run() {
            //Toast.makeText(getBaseContext(), "Timer Çalıştı " , Toast.LENGTH_SHORT).show();
            isBlock=false;
            // buraya ne yapmak istiyorsan o kodu yaz.. Kodun sonlandıktan sonra 1 saniye sonra tekrar çalışacak şekilde handler tekrar çalışacak.
            //mHandler.postDelayed(this, 1000);
        }
    };

    private void SetValue(String menuId,String value,String logTitle)
    {
        if (!Bluetooth.IsConnected()) {
            GotoConnect();
            return;
        }
        String code = "(#" + key + "#;" + SET_VALUES + ";" + menuId + "," + value + ")";
        Log.i(logTitle + " : ", code);
        Bluetooth.SendMessage(code);
    }

    boolean fanState=false;
    private void FanChange(boolean start)
    {
            if (start) {
                fanState=true;
                if(!hasRequest)
                    SetValue(FAN,"1","Fan Start");
                FanSpeedChange();
            }
            else {
                fanState=false;
                if(!hasRequest)
                    SetValue(FAN,"0","Fan Stop");
                gFunRunImage.setImageResource(R.drawable.bluefan);
            }
    }

    private void FanSpeedChange()
    {
        if(fanState)
        {
            int level=sbFanSpeed.getProgress()/85;
            switch(level)
            {
                case 0:
                {
                    gFunRunImage.setImageResource(R.drawable.bluefanrun1);
                    break;
                }
                case 1:
                {
                    gFunRunImage.setImageResource(R.drawable.bluefanrun2);
                    break;
                }
                case 2:
                {
                    gFunRunImage.setImageResource(R.drawable.bluefanrun3);
                    break;
                }
                default :
                {
                    gFunRunImage.setImageResource(R.drawable.bluefanrun3);
                    break;
                }
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        switch (item.getItemId()) {
            case R.id.connect:
                GotoConnect();
                return true;
            case R.id.close:
            {
                Bluetooth.Cancel();
                finish();
                System.exit(0);
                return true;
            }
            case R.id.reconnect:
            {
                GotoSplashScreen();
                return true;
            }
            default:
                return super.onOptionsItemSelected(item);
        }
    }
    private void GotoConnect()
    {
        Intent i = new Intent(getApplicationContext(),Connect.class);
        startActivity(i);
        finish();
    }

    private void GotoSplashScreen()
    {
        Intent i = new Intent(getApplicationContext(),SplashScreen.class);
        startActivity(i);
        finish();
    }

    private final BroadcastReceiver mBroadcastReceiver1 = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();

            if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)) {
                final int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
                switch(state) {
                    case BluetoothAdapter.STATE_OFF:
                        GotoConnect();
                        break;
                    case BluetoothAdapter.STATE_TURNING_OFF:

                        break;
                    case BluetoothAdapter.STATE_ON:

                        break;
                    case BluetoothAdapter.STATE_TURNING_ON:

                        break;
                }

            }
        }
    };

    @Override
    protected void onDestroy() {
        super.onDestroy();

        unregisterReceiver(mBroadcastReceiver1);
        unregisterReceiver(mBroadcastReceiver3);
    }

    private final BroadcastReceiver mBroadcastReceiver3 = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            switch (action){
                case BluetoothDevice.ACTION_ACL_CONNECTED:

                    break;
                case BluetoothDevice.ACTION_ACL_DISCONNECTED:
                    GotoConnect();
                    break;
            }
        }
    };

    private void PrepareHeater()
    {
        if(heaterTmp>Integer.parseInt(sFanStopTmp.getSelectedItem().toString())-Integer.parseInt(sFanToleranceTmp.getSelectedItem().toString()))
        {
            gHeaterImage.setImageResource(R.drawable.heater3);
        } else if (heaterTmp>Integer.parseInt(sFanStopTmp.getSelectedItem().toString())-Integer.parseInt(sFanToleranceTmp.getSelectedItem().toString())-15)
        {
            gHeaterImage.setImageResource(R.drawable.heater2);
        } else
        {
            gHeaterImage.setImageResource(R.drawable.heater1);
        }
    }

    boolean hasRequest=false;
    private void PrepareObject(Command c)
    {
        hasRequest=true;
        switch (c.o)
        {
            case  HEATER_TMP:
            {
                tvHeaterTmp.setText(c.v + " °C");
                heaterTmp=Integer.parseInt(c.v);
                PrepareHeater();
                break;
            }
            case  FAN:
            {
                if(Integer.parseInt(c.v)==1)
                    FanChange(true);
                else
                    FanChange(false);
                break;
            }
            case  FAN_SPEED:
            {
                sbFanSpeed.setProgress(Integer.parseInt(c.v));
                tvFanSpeedValue.setText(c.v);
                break;
            }
            case  FAN_STOP_TMP:
            {
                int spinnerPosition = Integer.parseInt(c.v) - 40;
                sFanStopTmp.setSelection(spinnerPosition);
                break;
            }
            case  FAN_TOLERANCE_TMP:
            {
                int spinnerPosition = Integer.parseInt(c.v) - 5;
                sFanToleranceTmp.setSelection(spinnerPosition);
                break;
            }
            case  POMPA_TMP:
            {
                int spinnerPosition = Integer.parseInt(c.v) - 10;
                sPompaTmp.setSelection(spinnerPosition);
                break;
            }
            default:
                throw new IllegalStateException("Unexpected value: " + c.o);
        }
        hasRequest=false;
    }
}
