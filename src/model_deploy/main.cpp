 #include "mbed.h"
#include "accelerometer_handler.h"
#include "config.h"
#include "magic_wand_model_data.h"
#include <cmath>
#include "DA7212.h"
//#include <stdlib.h>
#include "uLCD_4DGL.h"


#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"


DA7212 audio;
InterruptIn sw2(SW2); //setting
InterruptIn sw3(SW3); //confirm
uLCD_4DGL uLCD(D1, D0, D2);
Serial pc(USBTX, USBRX);

Thread t1;  //modeselect
Thread t2(osPriorityNormal, 120 * 1024);  //gesture
Thread t3;  //playing song
Thread t4;  //loadsignal
EventQueue queue1(32 * EVENTS_EVENT_SIZE);

int gesture_index; //0 ring, 1 slope, 2 N


int16_t waveform[kAudioTxBufferSize];
int song1[42] = {

  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261,
  392, 392, 349, 349, 330, 330, 294,
  392, 392, 349, 349, 330, 330, 294,
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 0
  };

int song2[31] = {

  247, 440, 494, 261, 261, 247, 392,
  392, 294, 294, 247, 261, 440, 440,
  440, 440, 440, 494, 261, 261, 247,
  440, 494, 261, 261, 247, 392, 330,
  330, 330, 0
  };

int song3[35] = {
  
  349, 330, 294, 261, 261, 392, 392,
  392, 392, 261, 261, 261, 440, 247,
  247, 349, 330, 294, 261, 261, 392,
  392, 392, 392, 261, 261, 261, 494,
  294, 247, 349, 330, 294, 261, 0
  };



int noteLength1[42] = {

  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2
  };

int noteLength2[31] = {

  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1
  };

int noteLength3[35] = {

  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1
  };

//
void playNote(int freq) {
    for(int i = 0; i < kAudioTxBufferSize; i++) {
        waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / freq)) * ((1<<16) - 1));
    }
    audio.spk.play(waveform, kAudioTxBufferSize);
}


 
int nowsong = 1;// 1 song1, 2 song2, 3 song3
int play = 0;//play or not

void modeselect(){
t3.start(callback(&queue1, &EventQueue::dispatch_forever));
  int nowmode = 1;//forward, 2 backward, 3 selectsong
  
  

  uLCD.cls();
  uLCD.color(RED);
  uLCD.printf("forward\n");
  uLCD.color(GREEN);
  uLCD.printf("backward\n");
  uLCD.printf("selectsong\n");
  //uLCD.printf("taiko\n");

  while(1){

     if(!sw3){
       wait(0.2);
        //nowmode = 1;
        //nowsong = 1;
        uLCD.cls();
        uLCD.color(GREEN);
        //uLCD.printf("song1 \n");
      while(nowmode == 1){
          while(nowsong == 1 && (gesture_index == 0 || gesture_index == 1 || gesture_index == 2)){
            nowsong = 3;
            uLCD.cls();
            uLCD.printf("song3 \n");
             
            gesture_index = 10;
          }
          while(nowsong == 2 && (gesture_index == 0 || gesture_index == 1 || gesture_index == 2)){
            nowsong = 1;
            uLCD.cls();
            uLCD.printf("song1 \n");
             
            gesture_index = 10;
          }
          while(nowsong == 3 && (gesture_index == 0 || gesture_index == 1 || gesture_index == 2)){
            nowsong = 2;
            uLCD.cls();
            uLCD.printf("song2 \n");
             
            gesture_index = 10;
          }

          while(!sw3){
            uLCD.cls();
            uLCD.printf("playing song %d \n", nowsong);
            play = 1;
            if(nowsong == 1 && play == 1){
      for(int i = 0; i < 42 && play == 1; i++){
      int length = noteLength1[i];
        while(length-- && play == 1){
          if(!sw2){
            play = 0;
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;

            for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, 0);
          }
          if(length < 1) wait(1.0);

            //queue1.call(playNote, 0);
          }
        // the loop below will play the note for the duration of 1s
        else{
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, song1[i]);
          }
          if(length < 1) wait(1.0);
          }
        }
      }
    }
    else if(nowsong == 2 && play == 1){
      for(int i = 0; i < 31 && play == 1; i++){
      int length = noteLength2[i];
         while(length-- && play == 1){
          if(!sw2){
            play = 0;
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;

            for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, 0);
          }
          if(length < 1) wait(1.0);

            //queue1.call(playNote, 0);
          }
        // the loop below will play the note for the duration of 1s
        else{
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, song2[i]);
          }
          if(length < 1) wait(1.0);
          }
        }
      }
    }
    else if(nowsong == 3 && play == 1){
      for(int i = 0; i < 35 && play == 1; i++){
      int length = noteLength3[i];
         while(length-- && play == 1){
          if(!sw2){
            play = 0;
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;

            for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, 0);
          }
          if(length < 1) wait(1.0);

            //queue1.call(playNote, 0);
          }
        // the loop below will play the note for the duration of 1s
        else{
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, song3[i]);
          }
          if(length < 1) wait(1.0);
          }
        }
      }
    }
          }

          while(!sw2){
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;
          } 
      }//mode
      while(nowmode == 2){
          while(nowsong == 1 && (gesture_index == 0 || gesture_index == 1 || gesture_index == 2)){
            nowsong = 2;
            uLCD.cls();
            uLCD.printf("song2 \n");
             
            gesture_index = 10;
          }
          while(nowsong == 2 && (gesture_index == 0 || gesture_index == 1 || gesture_index == 2)){
            nowsong = 3;
            uLCD.cls();
            uLCD.printf("song3 \n");
             
            gesture_index = 10;
          }
          while(nowsong == 3 && (gesture_index == 0 || gesture_index == 1 || gesture_index == 2)){
            nowsong = 1;
            uLCD.cls();
            uLCD.printf("song1 \n");
             
            gesture_index = 10;
          }

          while(!sw3){
            uLCD.cls();
            uLCD.printf("playing song %d \n", nowsong);
            play = 1;
            if(nowsong == 1 && play == 1){
      for(int i = 0; i < 42 && play == 1; i++){
      int length = noteLength1[i];
         while(length-- && play == 1){
          if(!sw2){
            play = 0;
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;

            for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, 0);
          }
          if(length < 1) wait(1.0);

            //queue1.call(playNote, 0);
          }
        // the loop below will play the note for the duration of 1s
        else{
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, song1[i]);
          }
          if(length < 1) wait(1.0);
          }
        }
      }
    }
    else if(nowsong == 2 && play == 1){
      for(int i = 0; i < 31 && play == 1; i++){
      int length = noteLength2[i];
         while(length-- && play == 1){
          if(!sw2){
            play = 0;
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;

            for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, 0);
          }
          if(length < 1) wait(1.0);

            //queue1.call(playNote, 0);
          }
        // the loop below will play the note for the duration of 1s
        else{
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, song2[i]);
          }
          if(length < 1) wait(1.0);
          }
        }
      }
    }
    else if(nowsong == 3 && play == 1){
      for(int i = 0; i < 35 && play == 1; i++){
      int length = noteLength3[i];
         while(length-- && play == 1){
          if(!sw2){
            play = 0;
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;

            for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, 0);
          }
          if(length < 1) wait(1.0);

            //queue1.call(playNote, 0);
          }
        // the loop below will play the note for the duration of 1s
        else{
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, song3[i]);
          }
          if(length < 1) wait(1.0);
          }
        }
      }
    }  
          }

          while(!sw2){
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;
          }
      }//mode2
      
      
      while(nowmode == 3 ){
        while(gesture_index == 0 ){
          uLCD.cls();
          nowsong = 1;
          uLCD.printf("song1 \n");
          
          gesture_index = 10;
        }
        while(gesture_index == 1 ){
          uLCD.cls();
          nowsong = 2;
          uLCD.printf("song2 \n");
           
          gesture_index = 10;
        }
        while(gesture_index == 2 ){
          uLCD.cls();
          nowsong = 3;
          uLCD.printf("song3 \n");
           
          gesture_index = 10;
        }

        while(!sw3){
          uLCD.cls();
          uLCD.printf("playing song %d \n", nowsong);
          play = 1;
          if(nowsong == 1 && play == 1){
      for(int i = 0; i < 42 && play == 1; i++){
      int length = noteLength1[i];
         while(length-- && play == 1){
          if(!sw2){
            play = 0;
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;

            for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, 0);
          }
          if(length < 1) wait(1.0);

            //queue1.call(playNote, 0);
          }
        // the loop below will play the note for the duration of 1s
        else{
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, song1[i]);
          }
          if(length < 1) wait(1.0);
          }
        }
      }
    }
    else if(nowsong == 2 && play == 1){
      for(int i = 0; i < 31 && play == 1; i++){
      int length = noteLength2[i];
         while(length-- && play == 1){
          if(!sw2){
            play = 0;
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;

            for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, 0);
          }
          if(length < 1) wait(1.0);

            //queue1.call(playNote, 0);
          }
        // the loop below will play the note for the duration of 1s
        else{
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, song2[i]);
          }
          if(length < 1) wait(1.0);
          }
        }
      }
    }
    else if(nowsong == 3 && play == 1){
      for(int i = 0; i < 35 && play == 1; i++){
      int length = noteLength3[i];
         while(length-- && play == 1){
          if(!sw2){
            play = 0;
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;

            for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, 0);
          }
          if(length < 1) wait(1.0);

            //queue1.call(playNote, 0);
          }
        // the loop below will play the note for the duration of 1s
        else{
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize ; ++j){
            queue1.call(playNote, song3[i]);
          }
          if(length < 1) wait(1.0);
          }
        }
      }
    }
        }

         while(!sw2){
            nowsong = 1;
            nowmode = 0;
            uLCD.cls();
            uLCD.color(RED);
            uLCD.printf("forward\n");
            uLCD.color(GREEN);
            uLCD.printf("backward\n");
            uLCD.printf("selectsong\n");
            gesture_index = 10;
          }
      }//mode3
    }//!sw3
    
    else if(gesture_index == 0 || gesture_index == 1 || gesture_index == 2 || nowmode == 0){
      if(nowmode == 0){
        uLCD.cls();
        uLCD.printf("%d\n", gesture_index);
        uLCD.color(RED);
        uLCD.printf("forward\n");
        uLCD.color(GREEN);
        uLCD.printf("backward\n");
        uLCD.color(GREEN);
        uLCD.printf("selectsong\n");
        
        
        //uLCD.printf("taiko\n");
        nowmode = 1;
        gesture_index = 10;
      }
      else if(nowmode == 1){
        uLCD.cls();
        uLCD.printf("%d\n", gesture_index);
        uLCD.color(GREEN);
        uLCD.printf("forward\n");
        uLCD.color(RED);
        uLCD.printf("backward\n");
        uLCD.color(GREEN);
        uLCD.printf("selectsong\n");
        
        
        //uLCD.printf("taiko\n");
        nowmode = 2;
        gesture_index = 10;
      }
      else if(nowmode == 2){
        uLCD.cls();
        uLCD.printf("%d\n", gesture_index);
        uLCD.color(GREEN);
        uLCD.printf("forward\n");
        uLCD.printf("backward\n");
        uLCD.color(RED);
        uLCD.printf("selectsong\n");
        //uLCD.printf("taiko\n");
        nowmode = 3;
        gesture_index = 10;
      }
      else if(nowmode == 3){
        uLCD.cls();
        uLCD.printf("%d\n", gesture_index);
        uLCD.color(RED);
        uLCD.printf("forward\n");
        uLCD.color(GREEN);
        uLCD.printf("backward\n");
        uLCD.printf("selectsong\n");
        //uLCD.printf("taiko\n");
        nowmode = 1;
        gesture_index = 10;
      }
    }//modeselect

  }//while end

}//void end      



//***************************************************gesture*****************************************************************

int PredictGesture(float* output) {
    // How many times the most recent gesture has been matched in a row
    static int continuous_count = 0;
    // The result of the last prediction
    static int last_predict = -1;

    // Find whichever output has a probability > 0.8 (they sum to 1)
    int this_predict = -1;
    for (int i = 0; i < label_num; i++) {
        if (output[i] > 0.8) this_predict = i;
    }

    // No gesture was detected above the threshold
    if (this_predict == -1) {
        continuous_count = 0;
        last_predict = label_num;
        return label_num;
    }

    if (last_predict == this_predict) {
        continuous_count += 1;
    } 
    else {
        continuous_count = 0;
    }
    last_predict = this_predict;

    // If we haven't yet had enough consecutive matches for this gesture,
    // report a negative result
    if (continuous_count < config.consecutiveInferenceThresholds[this_predict]) {
        return label_num;
    }
    // Otherwise, we've seen a positive result, so clear all our variables
    // and report it
    continuous_count = 0;
    last_predict = -1;

    return this_predict;
}
void gesture() {
    // Create an area of memory to use for input, output, and intermediate arrays.
    // The size of this will depend on the model you're using, and may need to be
    // determined by experimentation.
    constexpr int kTensorArenaSize = 60 * 1024;
    uint8_t tensor_arena[kTensorArenaSize];

    // Whether we should clear the buffer next time we fetch data
    bool should_clear_buffer = false;
    bool got_data = false;

    // The gesture index of the prediction

    //int gesture_index;

    // Set up logging.
    static tflite::MicroErrorReporter micro_error_reporter;
    tflite::ErrorReporter* error_reporter = &micro_error_reporter;

    // Map the model into a usable data structure. This doesn't involve any
    // copying or parsing, it's a very lightweight operation.
    const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        error_reporter->Report(
            "Model provided is schema version %d not equal "
            "to supported version %d.",
            model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    // Pull in only the operation implementations we need.
    // This relies on a complete list of all the ops needed by this graph.
    // An easier approach is to just use the AllOpsResolver, but this will
    // incur some penalty in code space for op implementations that are not
    // needed by this graph.
    static tflite::MicroOpResolver<6> micro_op_resolver;
    micro_op_resolver.AddBuiltin(
        tflite::BuiltinOperator_DEPTHWISE_CONV_2D,
        tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
    micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,
                               tflite::ops::micro::Register_MAX_POOL_2D());
    micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,
                               tflite::ops::micro::Register_CONV_2D());
    micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,
                               tflite::ops::micro::Register_FULLY_CONNECTED());
    micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,
                               tflite::ops::micro::Register_SOFTMAX());
    micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
                             tflite::ops::micro::Register_RESHAPE(), 1);
    // Build an interpreter to run the model with
    static tflite::MicroInterpreter static_interpreter(
        model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
    tflite::MicroInterpreter* interpreter = &static_interpreter;

    // Allocate memory from the tensor_arena for the model's tensors
    interpreter->AllocateTensors();

    // Obtain pointer to the model's input tensor
    TfLiteTensor* model_input = interpreter->input(0);
    if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
        (model_input->dims->data[1] != config.seq_length) ||
        (model_input->dims->data[2] != kChannelNumber) ||
        (model_input->type != kTfLiteFloat32)) {
            error_reporter->Report("Bad input tensor parameters in model");
        return;
    }

    int input_length = model_input->bytes / sizeof(float);

    TfLiteStatus setup_status = SetupAccelerometer(error_reporter);
    if (setup_status != kTfLiteOk) {
        error_reporter->Report("Set up failed\n");
        return;
    }

    error_reporter->Report("Set up successful...\n");

    while (true) {

        // Attempt to read new data from the accelerometer
        got_data = ReadAccelerometer(error_reporter, model_input->data.f,
                                 input_length, should_clear_buffer);

        // If there was no new data,
        // don't try to clear the buffer again and wait until next time
        if (!got_data) {
            should_clear_buffer = false;
            continue;
        }

        // Run inference, and report any error
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
            error_reporter->Report("Invoke failed on index: %d\n", begin_index);
            continue;
        }

        // Analyze the results to obtain a prediction
        gesture_index = PredictGesture(interpreter->output(0)->data.f);

        // Clear the buffer next time we read data
        should_clear_buffer = gesture_index < label_num;

        // Produce an output
         
        if (gesture_index < label_num) {
            error_reporter->Report(config.output_message[gesture_index]);  
        }
    }
}

//*************************************play song*************************************

/*void playsong(){

  uLCD.cls();
  uLCD.color(GREEN);
  uLCD.printf("now play song %d\n", nowsong);
  

} */      



/*void playingsong(){
  t3.start(callback(&queue, &EventQueue::dispatch_forever));
  while(1){
    if(nowsong == 1 && play == 1){
      for(int i = 0; i < 42 && play == 1; i++){
      int length = noteLength1[i];
        while(length-- && play == 1){
        // the loop below will play the note for the duration of 1s
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j){
            queue.call(playNote, song1[i]);
          }
          if(length < 1) wait(1.0);
        }
      }
    }
    else if(nowsong == 2 && play == 1){
      for(int i = 0; i < 35 && play == 1; i++){
      int length = noteLength2[i];
        while(length-- && play == 1){
        // the loop below will play the note for the duration of 1s
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j){
            queue.call(playNote, song2[i]);
          }
          if(length < 1) wait(1.0);
        }
      }
    }
    else if(nowsong == 3 && play == 1){
      for(int i = 0; i < 35 && play == 1; i++){
      int length = noteLength3[i];
        while(length-- && play == 1){
        // the loop below will play the note for the duration of 1s
          for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j){
            queue.call(playNote, song3[i]);
          }
          if(length < 1) wait(1.0);
        }
      }
    }
  }
  
}*/

void loadSignal() {
    int signalLength = 32;
    int bufferLength = 32;
    char serialInBuffer[bufferLength];
    int signal[signalLength] = {0};
    int i = 0;
    int serialCount = 0;
    
    // audio.spk.pause();
    while(i < signalLength) {
        if(pc.readable()) {
            //green_led = 0;
            serialInBuffer[serialCount] = pc.getc();
            serialCount++;
            if(serialCount == 3) {
                serialInBuffer[serialCount] = '\0';
                signal[i] = atoi(serialInBuffer);
                song1[i] = signal[i];
                pc.printf("%d\n\r", song1[i]);
                i++;
                serialCount = 0;
            }
        }
        //green_led = 1;
        
        // 
    }
    //green_led = 0; 
    /*for (int i = 0; i < 42; i++) {
        song1[i] = signal[i];
    }
    for (int i = 42; i < 77; i++) {
        song2[i-42] = signal[i];
    }
    for (int i = 77; i < 112; i++) {
        song3[i - 77] = signal[i];
    }
    wait(1.0); */   
}

   




int main(){
   
  t1.start(modeselect);
  t2.start(gesture);
  //t3.start(playingsong);
  t4.start(loadSignal);
}




 