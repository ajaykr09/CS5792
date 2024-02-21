package com.example.myapplication;

import androidx.appcompat.app.AppCompatActivity;

import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import java.io.BufferedInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.Socket;

public class MainActivity extends AppCompatActivity {

    private EditText ipAddressEditText, portEditText, fileLocationEditText;
    private Button connectButton, sendButton, disconnectButton;
    private TextView statusTextView;

    private Socket clientSocket;
    private DataOutputStream outputStream;

    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ipAddressEditText = findViewById(R.id.editTextIpAddress);
        portEditText = findViewById(R.id.editTextPort);
        fileLocationEditText = findViewById(R.id.editTextFileLocation);
        connectButton = findViewById(R.id.connectButton);
        sendButton = findViewById(R.id.buttonSend);
        disconnectButton = findViewById(R.id.buttonDisconnect);
        statusTextView = findViewById(R.id.statusTextView);

        connectButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                connectToServer();
            }
        });

        sendButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                sendData();
            }
        });

        disconnectButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                disconnectFromServer();
            }
        });
    }

    private void connectToServer() {
        String ipAddress = ipAddressEditText.getText().toString();
        int port = Integer.parseInt(portEditText.getText().toString());

        new ConnectToServerTask().execute(ipAddress, String.valueOf(port));
    }

    private void sendData() {
        String fileLocation = fileLocationEditText.getText().toString();

        if (clientSocket != null && clientSocket.isConnected()) {
            new SendDataTask().execute(fileLocation);
        } else {
            Log.e(TAG, "Socket is not connected.");
        }
    }

    private void disconnectFromServer() {
        if (clientSocket != null && clientSocket.isConnected()) {
            try {
                clientSocket.close();
                statusTextView.setText("Disconnected from server.");
            } catch (IOException e) {
                Log.e(TAG, "Error while disconnecting from server: " + e.getMessage());
            }
        }
    }

    private class ConnectToServerTask extends AsyncTask<String, Void, Void> {
        @Override
        protected Void doInBackground(String... params) {
            String ipAddress = params[0];
            int port = Integer.parseInt(params[1]);

            try {
                clientSocket = new Socket(ipAddress, port);
                outputStream = new DataOutputStream(clientSocket.getOutputStream());

                Log.d(TAG, "Connected to server.");
                statusTextView.setText("Connected to server.");
            } catch (IOException e) {
                Log.e(TAG, "Error while connecting to server: " + e.getMessage());
            }

            return null;
        }
    }

    private class SendDataTask extends AsyncTask<String, Void, Void> {
        @Override
        protected Void doInBackground(String... params) {
            String fileLocation = params[0];

            try {
                File file = new File(fileLocation);
                byte[] buffer = new byte[(int) file.length()];

                BufferedInputStream bis = new BufferedInputStream(new FileInputStream(file));
                bis.read(buffer, 0, buffer.length);

                // Send file size first
                outputStream.writeLong(file.length());
                outputStream.flush();

                // Send file contents
                outputStream.write(buffer, 0, buffer.length);
                outputStream.flush();

                Log.d(TAG, "Data sent successfully.");
                statusTextView.setText("Data sent successfully.");
            } catch (IOException e) {
                Log.e(TAG, "Error while sending data: " + e.getMessage());
            }

            return null;
        }
    }
}
