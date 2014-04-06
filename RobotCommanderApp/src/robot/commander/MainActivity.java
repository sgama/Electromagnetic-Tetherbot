package robot.commander;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
import android.view.View.*;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;

public class MainActivity extends Activity {
	
	private Button mFollowButton;
	private Button mForwardButton;
	private Button mBackwardButton;
	private Button mRotateButton;
	private Button mParkButton;
	
	private AudioTrack tone;
	private int sample_rate;
	private short[] samples;
	private boolean pressed = false;
	private double frequencyHz;
	private Thread tonethread;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		 super.onCreate(savedInstanceState);
		 setContentView(R.layout.activity_main);
		 
		 sample_rate = 22000;
		 tone = new AudioTrack(AudioManager.STREAM_MUSIC, sample_rate,
					AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_DEFAULT,
					sample_rate, AudioTrack.MODE_STREAM);
		 
		 
		 mFollowButton = (Button)findViewById(R.id.mFollowButton);
		 mFollowButton.setOnTouchListener(new OnTouchListener() {
			 @Override
			 public boolean onTouch(View v, MotionEvent event) {
				 if(event.getAction() == MotionEvent.ACTION_DOWN) {
					 playTone(4200.0); 
				 }
				 else if(event.getAction() == MotionEvent.ACTION_UP) {
					 resetTone();
				 }
				 return false;
			 }
		 });
		 
		 mForwardButton = (Button)findViewById(R.id.mForwardButton);
		 mForwardButton.setOnTouchListener(new OnTouchListener() {
			 @Override
			 public boolean onTouch(View v, MotionEvent event) {
				 if(event.getAction() == MotionEvent.ACTION_DOWN) {
					 playTone(4400.0); 
				 }
				 else if(event.getAction() == MotionEvent.ACTION_UP) {
					 resetTone();
				 }
				 return false;
			 }
		 });
		 
		 mBackwardButton = (Button)findViewById(R.id.mBackwardButton);
		 mBackwardButton.setOnTouchListener(new OnTouchListener() {
			 @Override
			 public boolean onTouch(View v, MotionEvent event) {
				 if(event.getAction() == MotionEvent.ACTION_DOWN) {
					 playTone(4600.0); 
				 }
				 else if(event.getAction() == MotionEvent.ACTION_UP) {
					 resetTone();
				 }
				 return false;
			 }
		 });
		 
		 mRotateButton = (Button)findViewById(R.id.mRotateButton);
		 mRotateButton.setOnTouchListener(new OnTouchListener() {
			 @Override
			 public boolean onTouch(View v, MotionEvent event) {
				 if(event.getAction() == MotionEvent.ACTION_DOWN) {
					 playTone(4800.0); 
				 }
				 else if(event.getAction() == MotionEvent.ACTION_UP) {
					 resetTone();
				 }
				 return false;
			 }
		 });
		 
		 mParkButton = (Button)findViewById(R.id.mParkButton);
		 mParkButton.setOnTouchListener(new OnTouchListener() {
			 @Override
			 public boolean onTouch(View v, MotionEvent event) {
				 if(event.getAction() == MotionEvent.ACTION_DOWN) {
					 playTone(5000.0); 
				 }
				 else if(event.getAction() == MotionEvent.ACTION_UP) {
					 resetTone();
				 }
				 return false;
			 }
		 });
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		//getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	protected void onResume() {
		super.onResume();
	}
		
	Runnable genTone = new Runnable() {       
        public void run()
        {
        	Thread.currentThread().setPriority(Thread.MIN_PRIORITY);
		 
        	samples = new short[sample_rate];
		
        	for(int i = 0; i < sample_rate; i += 1){
        		short sample = (short)(Math.sin(2 * Math.PI * i / ((double)sample_rate / (frequencyHz))) * 32767);
        		samples[i + 0] = sample;
        	}
        	
        	while(pressed){
        		tone.write(samples, 0, sample_rate);
        	}
        }
	};
	
	private void resetTone(){
		pressed = false;
		tone.pause();
		tone.flush();
	}
	
	private void playTone(double frequency){
		pressed = true;
		frequencyHz = frequency;
		tonethread = new Thread(genTone);
		tonethread.start();
		tone.play();
	}
}
