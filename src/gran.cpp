#include "plugin.hpp"
#include "samplerate.h"
#include <math.h>

const long HISTORY_SIZE = 1<<21;
const double HISTORY_SIZE_F = (double) HISTORY_SIZE;

long mod(long a, long b){
    return (a % b + b) % b; 
}


template <long S>
struct CircularArray{
     double data[2*S] = {0.0};
     long write_head = 1;
     long read_head = 1;

     void push(double t){
        data[write_head] = t; 
        write_head++;
        if (write_head == 2*S){
        	write_head = 0;
        }
     }

     void set_read_head(long j, long n){
     	 read_head = mod(write_head-j-n-1, 2*S);
     }
     
     double read_at(long j){
     	long d = (read_head + j)%(2*S);
     	//printf("(read_head + j)%(2*S) = %ld\n", d);
     	//printf("write_head = %ld\n", write_head);
        return data[d];
     }
};

double hamming(double n){
    return (25.0 + 21*rack::simd::sin(M_PI*n))/46.0;
}

struct GraN : Module {
	enum ParamIds {
		START_MIN_PARAM,
		START_RANGE_PARAM,
		LENGTH_MIN_PARAM,
		LENGTH_RANGE_PARAM,
		TIME_MIN_PARAM,
		TIME_RANGE_PARAM,
		SPEED_MIN_PARAM,
		SPEED_RANGE_PARAM,
		PAN_MIN_PARAM,
		PAN_RANGE_PARAM,
		VOL_MIN_PARAM,
		VOL_RANGE_PARAM,
		RESET_PARAM,
		NUM_PARAMS		
	};
	enum InputIds {
		MONO_INPUT,
		START_MIN_INPUT,
		START_RANGE_INPUT,
		LENGTH_MIN_INPUT,
		LENGTH_RANGE_INPUT,
		TIME_MIN_INPUT,
		TIME_RANGE_INPUT,
		SPEED_MIN_INPUT,
		SPEED_RANGE_INPUT,
		PAN_MIN_INPUT,
		PAN_RANGE_INPUT,
		VOL_MIN_INPUT,
		VOL_RANGE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		LEFT_OUTPUT,
		RIGHT_OUTPUT,
		NUM_OUTPUTS
	};

	enum LightIds {
		RESET_LIGHT,
		NUM_LIGHTS
	};

	CircularArray<HISTORY_SIZE> historyBuffer;
	long out_idx = 0;
	long N = 1;
	double increment = 1;
	double div = 1;
	double panning=0.5;
    bool set_head=true;
	double wet = 0.0;
	long wait = 0;
	bool wait_for = false;
	double vol = 1;
	dsp::SchmittTrigger resetTrigger;

	GraN() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(START_MIN_PARAM, 0.f, 1.f, 0.1f, "");
		configParam(START_RANGE_PARAM, 0.f, 1.f, 0.01f, "");
		configParam(LENGTH_MIN_PARAM, 0.f, 1.f, 0.01f, "");
		configParam(LENGTH_RANGE_PARAM, 0.f, 1.f, 0.01f, "");
		configParam(TIME_MIN_PARAM, 0.f, 1.f, 0.0f, "");
		configParam(TIME_RANGE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SPEED_MIN_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(SPEED_RANGE_PARAM, 0.f, 1.f, 0.0f, "");
		configParam(PAN_MIN_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(PAN_RANGE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(RESET_PARAM, 0.f, 1.f, 0.f, "");

	}

    double randrange(double min, double range, bool normal){
    	if (normal){
    		return rack::random::normal()*range + min;
    	}else{
    		return rack::random::uniform()*range + min;
    	}
        
    }

	void process(const ProcessArgs& args) override {
        // Compute the frequency from the pitch parameter and input

        //double speed = (rack::random::normal()*speed_range) + speed_min;
        //double vol = (rack::random::normal()*vol_range) + vol_min;


        if (resetTrigger.process(params[RESET_PARAM].getValue())){
        	set_head = true;
        	wait_for = false;
        	wait = 0;
        }

		if (set_head){
        	if (wait > 0){
        		wait --;
        		outputs[LEFT_OUTPUT].setVoltage(0);
        		outputs[RIGHT_OUTPUT].setVoltage(0);
        		return ;
        	}
			if (wait_for){
		        double time_min = params[TIME_MIN_PARAM].getValue();
        		//if (inputs[START_MIN_INPUT].isConnected()){
        		//    start_min = inputs[START_MIN_INPUT].getVoltage();
        		//}

		        double time_range = params[TIME_RANGE_PARAM].getValue();   
				wait = (long) std::max(randrange(time_min, time_range, true)*10000, 0.0);
				wait_for = false;
			}
		}

        //dsp::SchmittTrigger resetTrigger;
        //resetTrigger.process(params[RESET_PARAM].getValue())



		if (set_head) {
            double start_min = params[START_MIN_PARAM].getValue();
            //if (inputs[START_MIN_INPUT].isConnected()){
            //    start_min = inputs[START_MIN_INPUT].getVoltage();
            //}

            double start_range = params[START_RANGE_PARAM].getValue();
            //if (inputs[START_RANGE_INPUT].isConnected()){
            //    start_range += inputs[START_RANGE_INPUT].getVoltage();
            //}

            long j = std::max((long) (randrange(start_min*HISTORY_SIZE_F/10, start_range*HISTORY_SIZE_F/10000, true)), (long) 0);
            // Copy from the index

            double length_min = params[LENGTH_MIN_PARAM].getValue();
            //if (inputs[LENGTH_MIN_INPUT].isConnected()){
            //    length_min += inputs[LENGTH_MIN_INPUT].getVoltage();
            //}

            double length_range = params[LENGTH_RANGE_PARAM].getValue();
            //if (inputs[LENGTH_RANGE_INPUT].isConnected()){
            //    length_range += inputs[LENGTH_RANGE_INPUT].getVoltage();
            //}
            //printf("length_min=%f, length_range=%f\n", length_min, length_range);
            long n = (long) std::max(randrange((length_min*HISTORY_SIZE_F/4)-length_range*HISTORY_SIZE_F/8, length_range*HISTORY_SIZE_F/8, true), 0.0);
            historyBuffer.set_read_head(j, n);
            out_idx = 0;
            N = n;
            
            //vol == panning
            double pan_min = params[PAN_MIN_PARAM].getValue();
            double pan_range = params[PAN_RANGE_PARAM].getValue();
            if (inputs[PAN_MIN_INPUT].isConnected()){
                pan_min  += inputs[PAN_MIN_INPUT].getVoltage();
            }
            if (inputs[PAN_RANGE_INPUT].isConnected()){
                pan_range  += inputs[PAN_RANGE_INPUT].getVoltage();
            }
            panning = std::max(std::min(randrange(pan_min-pan_range, 2*pan_range, true), 1.0), 0.0);
            //printf("panning = %f, length = %ld, start = %ld\n", pan_min, n, j);
            set_head = false;
            //speed -> increment 0.1 <- 10 times kai meta long h increment 10 gia 1/10 kai meta long

	        double speed_min = params[SPEED_MIN_PARAM].getValue();
	        //if (inputs[SPEED_MIN_INPUT].isConnected()){
	        //    speed_min += inputs[SPEED_MIN_INPUT].getVoltage();
	        //}

	        double speed_range = params[SPEED_RANGE_PARAM].getValue()/10;
	        //if (inputs[SPEED_RANGE_INPUT].isConnected()){
	        //    speed_range += inputs[SPEED_RANGE_INPUT].getVoltage();
	        //}
	        
	        // or powers of two
	        double speed = std::max(std::min(randrange(speed_min-speed_range, 2*speed_range, true), 1.0), 0.0)-0.5;
	        if (speed > 0){
	        	increment = (std::max(speed*49, 0.0)+ 1.0);
	        	div = 1.0;
	        }else{
	        	increment = 1.0;
	        	div = std::max(-speed*8, 0.0) + 1.0;
	        }
	        // knob 
	        // if powers of 2
	        // map between 
	        // 1 << speed
	        // else
			wait_for = true;

			double vol_min = params[VOL_MIN_PARAM].getValue();
	        //if (inputs[SPEED_MIN_INPUT].isConnected()){
	        //    speed_min += inputs[SPEED_MIN_INPUT].getVoltage();
	        //}

	        double vol_range = params[VOL_RANGE_PARAM].getValue()/10;
	        //if (inputs[SPEED_RANGE_INPUT].isConnected()){
	        //    speed_range += inputs[SPEED_RANGE_INPUT].getVoltage();
	        //}

			vol = std::max(std::min(randrange(vol_min - vol_range, 2*vol_range, true), 1.0), 0.0);
	        //printf("div %f, increment %f\n", div, increment);
		}
		// If out buffer is not empty get next
		long idx = (long)(out_idx/div);
		if (N > 0){
			//printf("h %f%f\n",  ((double) out_idx)/N, hamming(((double) out_idx)/N));
			wet = vol*historyBuffer.read_at(idx)*hamming(idx/N);
		}else{
			wet = vol*historyBuffer.read_at(idx);
		}
		//printf("%f\n", wet);
		if (idx < N){
			out_idx += increment;
		}else{
			set_head = true;
		}

        if(inputs[MONO_INPUT].isConnected()){
			historyBuffer.push(inputs[MONO_INPUT].getVoltage());
        }

		outputs[LEFT_OUTPUT].setVoltage((1-panning)*wet);
        outputs[RIGHT_OUTPUT].setVoltage(panning*wet);
		lights[RESET_LIGHT].setSmoothBrightness(resetTrigger.isHigh(), args.sampleTime);
	}
};


struct GraNWidget : ModuleWidget {
	GraNWidget(GraN* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/gran.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 15, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 15, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        float X_MIN = 16;
        float X_RANGE = 36;

		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN, 24)), module, GraN::START_MIN_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_RANGE, 24)), module, GraN::START_RANGE_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN, 45)), module, GraN::LENGTH_MIN_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_RANGE, 45)), module, GraN::LENGTH_RANGE_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN, 64)), module, GraN::TIME_MIN_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_RANGE, 64)), module, GraN::TIME_RANGE_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN, 80)), module, GraN::SPEED_MIN_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_RANGE, 80)), module, GraN::SPEED_RANGE_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN, 96)), module, GraN::PAN_MIN_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_RANGE, 96)), module, GraN::PAN_RANGE_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN, 111)), module, GraN::VOL_MIN_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_RANGE, 111)), module, GraN::VOL_RANGE_PARAM));
		addParam(createParam<LEDButton>(mm2px(Vec(RACK_GRID_WIDTH-12.2, 8.7)), module, GraN::RESET_PARAM));
		addChild(createLight<MediumLight<GreenLight>>(mm2px(Vec(RACK_GRID_WIDTH-10.7, 10.1)), module, GraN::RESET_LIGHT));


        float X_S = X_MIN+10;
        float X_R = X_RANGE+10;

        /* PARAM INPUTS STACK */
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_S, 24)), module, GraN::START_MIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_R, 24)), module, GraN::START_RANGE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_S, 45)), module, GraN::LENGTH_MIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_R, 45)), module, GraN::LENGTH_RANGE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_S, 64)), module, GraN::TIME_MIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_R, 64)), module, GraN::TIME_RANGE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_S, 80)), module, GraN::SPEED_MIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_R, 80)), module, GraN::SPEED_RANGE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_S, 96)), module, GraN::PAN_MIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_R, 96)), module, GraN::PAN_RANGE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_S, 111)), module, GraN::VOL_MIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_R, 111)), module, GraN::VOL_RANGE_INPUT));

        float JACK_Y = 121.8;
        // IN L
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.5, JACK_Y)), module, GraN::MONO_INPUT));
        // IN R
		//addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19, JACK_Y)), module, GraN::RIGHT_INPUT));


        // OUT L
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31.8, JACK_Y)), module, GraN::LEFT_OUTPUT));
        // OUT R
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(40.8, JACK_Y)), module, GraN::RIGHT_OUTPUT));

	}
};


Model* modelGraN = createModel<GraN, GraNWidget>("GraN");


/*

        float start = randrange()

        float length_min = params[LENGTH_MIN_PARAM].getValue();
        if (inputs[LENGTH_MIN_INPUT].isConnected()){
            length_min += inputs[LENGTH_MIN_INPUT].getVoltage();
        }
        float length_range = params[LENGTH_RANGE_PARAM].getValue();
        if (inputs[LENGTH_RANGE_INPUT].isConnected()){
            length_range += inputs[LENGTH_RANGE_INPUT].getVoltage();
        }

        float length = (rack::random::normal()*length_range) + length_min

        float time_min = params[TIME_MIN_PARAM].getValue();
        if (inputs[TIME_MIN_INPUT].isConnected()){
            length_min += inputs[TIME_MIN_INPUT].getVoltage();
        }

        float time_range = params[TIME_RANGE_PARAM].getValue();
        if (inputs[TIME_RANGE_INPUT].isConnected()){
            time_range += inputs[TIME_RANGE_INPUT].getVoltage();
        }

        float time = (rack::random::normal()*time_range) + time_min

        float speed_min = params[SPEED_MIN_PARAM].getValue();
        if (inputs[SPEED_MIN_INPUT].isConnected()){
            speed_min += inputs[SPEED_MIN_INPUT].getVoltage();
        }

        float speed_range = params[SPEED_RANGE_PARAM].getValue();
        if (inputs[SPEED_RANGE_INPUT].isConnected()){
            speed_range += inputs[SPEED_RANGE_INPUT].getVoltage();
        }
*/
