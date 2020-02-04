#include "plugin.hpp"
#include "samplerate.h"
#include <math.h>


struct Rabits : Module {
	enum ParamIds {
		R_PARAM,
		ROC_PARAM,
		VOL_PARAM,
        RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		R_INPUT,
		ROC_INPUT,
		VOL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT,
		NUM_OUTPUTS
	};

    enum LightIds {
		RESET_LIGHT,
		ENUMS(PHASE_LIGHT, 3),
		NUM_LIGHTS
	};

    float phase=0;
    double xn = 0.1;
    double xn_1 = 0.1;
    bool reset=true;
    bool is_reset=true;
    dsp::ClockDivider lightDivider;
	dsp::SchmittTrigger resetTrigger;

	Rabits() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(R_PARAM, 3.f, 4.f, 3.f, "r");
		configParam(ROC_PARAM, -100.f, 54.f, 0.f, "Frequency", "Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(VOL_PARAM, 0.f, 10.f, 0.0001f, "VOL");
		configParam(RESET_PARAM, 0.f, 1.f, 0.f, "Reset");
		lightDivider.setDivision(16);
	}


	double rabbits(double x, float r){
	    return x*(1-x)*r;
	}

    double lerp(double a, double b, double f){
        return a + f * (b - a);
    }

	void process(const ProcessArgs& args) override {
		// Advance phase
		bool reset = resetTrigger.process(params[RESET_PARAM].getValue());
		double pitch = params[ROC_PARAM].getValue() / 12.f;
        pitch += inputs[ROC_INPUT].getVoltage();

        pitch = dsp::FREQ_C4 * (pow(2, pitch+30) / 1073741824);

		float deltaPhase = simd::clamp(pitch * args.sampleTime, 1e-6f, 0.35f);
		phase += deltaPhase;
		bool new_sample = (phase >= 1);
		phase -= simd::floor(phase);

        if((!outputs[OUTPUT].isConnected()) || reset){
            xn_1 = xn;
        }

        if(outputs[OUTPUT].isConnected()){
            if (new_sample){
                xn_1 = xn;
                float r = params[R_PARAM].getValue() + inputs[R_INPUT].getVoltage();
                xn = rabbits(xn_1, r);
            }
            double out = 10.0*lerp(xn_1, xn, (double) phase)-5.0;
            //printf("%s\n", );
        	outputs[OUTPUT].setVoltage((params[VOL_PARAM].getValue()+inputs[VOL_INPUT].getVoltage())*out);
        }

		if (lightDivider.process()) {
			float lightValue = simd::sin(2 * M_PI * phase);
			lights[PHASE_LIGHT + 0].setSmoothBrightness(-lightValue, args.sampleTime * lightDivider.getDivision());
			lights[PHASE_LIGHT + 1].setSmoothBrightness(lightValue, args.sampleTime * lightDivider.getDivision());
			lights[PHASE_LIGHT + 2].setBrightness(0.f);
		}
		lights[RESET_LIGHT].setSmoothBrightness(resetTrigger.isHigh(), args.sampleTime);
	}
};


struct RabitsWidget : ModuleWidget {
	RabitsWidget(Rabits* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rabits.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 15, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 15, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        float X_MIN = 13;

		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN, 27)), module, Rabits::R_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN, 53)), module, Rabits::ROC_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN, 80)), module, Rabits::VOL_PARAM));
		addParam(createParam<LEDButton>(mm2px(Vec(RACK_GRID_WIDTH-4.7, 8.7+95)), module, Rabits::RESET_PARAM));
		addChild(createLight<MediumLight<GreenLight>>(mm2px(Vec(RACK_GRID_WIDTH-3.2, 10.1+95)), module, Rabits::RESET_LIGHT));


        float sep = 10;
        /* PARAM INPUTS STACK */
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN, 27 + sep)), module, Rabits::R_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN, 53 + sep)), module, Rabits::ROC_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN, 80+sep)), module, Rabits::VOL_INPUT));;

        float JACK_Y = 121.0;
        // OUT
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(13, JACK_Y)), module, Rabits::OUTPUT));
        addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(X_MIN+46, 150), module, Rabits::PHASE_LIGHT));
	}
};


Model* modelRabits = createModel<Rabits, RabitsWidget>("Rabits");
