#include "plugin.hpp"
#include "samplerate.h"
#include <math.h>


struct Frac : Module {
	enum ParamIds {
		FREQ_PARAM,
		K_PARAM,
		H_PARAM,
		G_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH_INPUT,
		K_INPUT,
		H_INPUT,
		G_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		WEIRSTRASS_OUTPUT,
		TAGAKI_OUTPUT,
		NUM_OUTPUTS
	};

    enum LightIds {
		ENUMS(PHASE_LIGHT, 3),
		NUM_LIGHTS
	};

    float phase=0;
    dsp::ClockDivider lightDivider;

	Frac() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FREQ_PARAM, -54.f, 54.f, 0.f, "Frequency", "Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(K_PARAM, 0.f, 1.f, 0.01f, "");
		configParam(H_PARAM, 0.f, 1.f, 0.01f, "");
		configParam(G_PARAM, 0.f, 1.f, 0.01f, "");
		lightDivider.setDivision(16);
	}


	double wcf(int k_max, double H, int gamma, double t){
		//weierstrass_cosine_function
		double sum = 0;
		for (int k=0; k<=k_max; k++){
			double a = pow(gamma, k);
			sum += pow(a, -H)*rack::simd::sin(M_PI*a*t);
		}
		return sum;
	}

	double s_tri(double x){
		return std::min(ceil(x) - x, x - floor(x));
	}

	double tagaki(int k_max, double t){
		//weierstrass_cosine_function
		double sum = s_tri(t);
		for (int k=1; k<=k_max; k++){
			double tri = s_tri(ldexp(t, k));
			sum += ldexp(tri, -k);
			//double tri = s_tri(t / pow(2,k));
			//sum += tri*pow(2,-k);
		}
		return sum;
	}

	void process(const ProcessArgs& args) override {
		// Advance phase
		double pitch = params[FREQ_PARAM].getValue() / 12.f;
        pitch += inputs[PITCH_INPUT].getVoltage();

        // Accumulate the phase
        // dsp::approxExp2_taylor5(pitch+30)
        pitch = dsp::FREQ_C4 * (pow(2, pitch+30) / 1073741824);

		float deltaPhase = simd::clamp(pitch * args.sampleTime, 1e-6f, 0.35f);
		phase += deltaPhase;
		phase -= simd::floor(phase);
        if(outputs[WEIRSTRASS_OUTPUT].isConnected()){
			double H = 1.0-params[H_PARAM].getValue();
			int k = std::max((int) (params[K_PARAM].getValue()*10),0);
			int gamma = std::max((int) (params[G_PARAM].getValue()*20),1);
			double output = wcf(k, H, gamma, phase);
			if (gamma == 1){
        		output /= 2.0;
        	}
        	outputs[WEIRSTRASS_OUTPUT].setVoltage(output);

        }
        
        if(outputs[TAGAKI_OUTPUT].isConnected()){
			int k = std::max((int) (params[K_PARAM].getValue()*10),0);
			//printf("%f\n", tagaki(k, phase));
        	outputs[TAGAKI_OUTPUT].setVoltage((tagaki(k, phase)-0.1)*2);
        }

		if (lightDivider.process()) {
			float lightValue = simd::sin(2 * M_PI * phase);
			lights[PHASE_LIGHT + 0].setSmoothBrightness(-lightValue, args.sampleTime * lightDivider.getDivision());
			lights[PHASE_LIGHT + 1].setSmoothBrightness(lightValue, args.sampleTime * lightDivider.getDivision());
			lights[PHASE_LIGHT + 2].setBrightness(0.f);
		}
	}
};


struct FracWidget : ModuleWidget {
	FracWidget(Frac* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/frac.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 15, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 15, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        float X_MIN = 13;

		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN, 27)), module, Frac::FREQ_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN, 53)), module, Frac::K_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN-5, 80)), module, Frac::H_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN+5, 80)), module, Frac::G_PARAM));

        float sep = 10;
        /* PARAM INPUTS STACK */
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN, 27 + sep)), module, Frac::PITCH_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN, 53 + sep)), module, Frac::K_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN-5, 80 + sep)), module, Frac::H_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN+5, 80 + sep)), module, Frac::G_INPUT));

        float JACK_Y = 120.8;
        // OUT L
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8., JACK_Y)), module, Frac::TAGAKI_OUTPUT));
        // OUT R
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17, JACK_Y)), module, Frac::WEIRSTRASS_OUTPUT));
        addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(X_MIN+40, 48), module, Frac::PHASE_LIGHT));
	}
};


Model* modelFrac = createModel<Frac, FracWidget>("Frac");
