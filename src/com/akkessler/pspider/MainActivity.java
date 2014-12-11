package com.akkessler.pspider;

import android.app.ProgressDialog;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.ActionBarActivity;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;


public class MainActivity extends ActionBarActivity implements OnClickListener {

	private EditText txtSeed, txtKeywords, txtPort, txtRequest, txtDepth, txtSpiders;
	private EditText txtDebug;
	private ListView list;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        this.txtSeed = (EditText) super.findViewById(R.id.txtSeed);
        //this.txtKeywords = (EditText) super.findViewById(R.id.txtKeywords);
        //this.txtPort = (EditText) super.findViewById(R.id.txtPort);
        //this.txtRequest = (EditText) super.findViewById(R.id.txtRequest);
        this.txtDepth = (EditText) super.findViewById(R.id.txtDepth);
        this.txtSpiders = (EditText) super.findViewById(R.id.txtSpiders);
        this.txtDebug = (EditText) super.findViewById(R.id.debug);
        this.list = (ListView) super.findViewById(R.id.list);
        Button button = (Button) super.findViewById(R.id.btnCrawl);
        button.setOnClickListener(this);
    }

    @Override
	public void onClick(View v) {
		final String seed = this.txtSeed.getText().toString();
		//final String keywords = this.txtKeywords.getText().toString();
		//final String request = this.txtRequest.getText().toString();
		//final String port = this.txtPort.getText().toString();
		final int depth = Integer.parseInt(this.txtDepth.getText().toString());
		final int spiders = Integer.parseInt(this.txtSpiders.getText().toString());
		final ProgressDialog dialog = ProgressDialog.show(this, "", "Crawling the web...", true);
		new AsyncTask<Void, Void, String>(){

			@Override
			protected String doInBackground(Void... params) {
				String result = pspider.nativeMain(seed, depth, spiders);
				return result;
			}
	
			@Override
			protected void onPostExecute(String result) {
				dialog.dismiss();
				MainActivity.this.txtDebug.setText(result);
			}
		}.execute();
	}
}
