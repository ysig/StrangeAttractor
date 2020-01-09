#include "plugin.hpp"
#include <math.h>


struct Attractor : Module {
	enum ParamIds {
		A_PARAM,
		B_PARAM,
		C_PARAM,
		D_PARAM,
		SELECT_PARAM,
		CLOCK_PARAM,
		ROC_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		A_INPUT,
		B_INPUT,
		C_INPUT,
		D_INPUT,
		CLOCK_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ATTRACTOR_X,
		ATTRACTOR_Y,
		NUM_OUTPUTS
	};

    /*enum LightIds {
		ENUMS(PHASE_LIGHT, 3),
		NUM_LIGHTS
	};*/

    float phase=0;
    float phase_roc=0;
    double x=0;
    double y=0;
    double xl=0;
    double yl=0;
    long count=1;
    dsp::ClockDivider lightDivider;
	dsp::SchmittTrigger clockTrigger;

	enum AttractorE {Clifford, De_Jong, Svensson, Bedhead, Dream, HopalongA, HopalongB, Gumowski_Mira};
	enum AttractorE current_attractor = (enum AttractorE) 0;

	Attractor() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);//, NUM_LIGHTS);
		configParam(A_PARAM, -2.f, 2.f, -1.4f, "a");
		configParam(B_PARAM, -2.f, 2.f, 1.6f, "b");
		configParam(C_PARAM, -2.f, 2.f, 1.0f, "c");
		configParam(D_PARAM, -2.f, 2.f, 0.7f, "d");
		configParam(CLOCK_PARAM, -2.f, 6.f, 2.f, "Clock tempo", " bpm", 2.f, 60.f);
		configParam(ROC_PARAM, -2.f, 12.f, 2.f, "RoC", " samples");
		configParam(SELECT_PARAM, 0.f, 7.f, 0.f, "Select");
		lightDivider.setDivision(16);
	}

	double G(double x, double mu){
		return mu * x + 2 * (1 - mu) * pow(x, 2) / (1.0 + pow(x, 2));
	}

	void process(const ProcessArgs& args) override {
		// Advance phase
        if(outputs[ATTRACTOR_X].isConnected() && outputs[ATTRACTOR_Y].isConnected()){
			float a = params[A_PARAM].getValue() + inputs[A_INPUT].getVoltage();
			float b = params[B_PARAM].getValue() + inputs[B_INPUT].getVoltage();
			float c = params[C_PARAM].getValue() + inputs[C_INPUT].getVoltage();
			float d = params[D_PARAM].getValue() + inputs[D_INPUT].getVoltage();

			bool gateIn = false;
			if (inputs[CLOCK_INPUT].isConnected()) {
				// External clock
				if(clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())){
					phase = 0.f;
				};
				gateIn = clockTrigger.isHigh();
			}
			else {
				// Internal clock
				float clockTime = std::pow(2.f, params[CLOCK_PARAM].getValue());
				phase += clockTime * args.sampleTime;
				if (phase >= 1.f) {
					phase = 0.f;
					gateIn = true;
				}
			}

			if (gateIn){
	    		xl = x;
	    		yl = y;
	    	}
	    	outputs[ATTRACTOR_X].setVoltage(xl);
	    	outputs[ATTRACTOR_Y].setVoltage(yl);

			phase_roc += std::pow(2.f, params[ROC_PARAM].getValue()) * args.sampleTime;
			if (phase_roc >= 1.f) {
				phase_roc = 0.f;
    			enum AttractorE attractor = (enum AttractorE) (int) params[SELECT_PARAM].getValue();
    			if (current_attractor != attractor){
    				x = 0;
    				y = 0;
    				current_attractor = attractor;
    			}

				double xp = x;
				switch(current_attractor)
				{
				    case Clifford: x = sin(a * y) + c*cos(a * xp); y = sin(b * xp) + d*cos(b * y); break;
				    case De_Jong: x = sin(a * y) - cos(b * xp); y = sin(c * xp) - cos(d * y); break;
				    case Svensson: x = d * sin(a * xp) - sin(b * y); y = c * cos(a * xp) + cos(b * y); break;
				    case Bedhead: x = sin(xp*y/b)*y + cos(a*xp-y); y = xp + sin(y)/b; break;
				    case Dream: x = sin(y*b)+c*sin(xp*b); y = sin(xp*a)+d*sin(y*a); break;
				    case HopalongA: x = y + (xp >= 0? 1.0 : -1.0)*sqrt(abs(b * xp - c)); y = a - xp ; break;
				    case HopalongB: x = y - 1.0 + (xp >= 0? 1.0 : -1.0)*sqrt(abs(b * xp - 1.0 - c)); y = a - xp - 1.0; break;
				    case Gumowski_Mira: x = y + a*(1 - b*pow(y,2))*y + G(xp, c); y = -xp + G(x, c); break;
				}
				count = 1;
			}
        }

		/*if (lightDivider.process()) {
			float lightValue = simd::sin(2 * M_PI * phase);
			lights[PHASE_LIGHT + 0].setSmoothBrightness(-lightValue, args.sampleTime * lightDivider.getDivision());
			lights[PHASE_LIGHT + 1].setSmoothBrightness(lightValue, args.sampleTime * lightDivider.getDivision());
			lights[PHASE_LIGHT + 2].setBrightness(0.f);
		}*/
	}
};


struct AttractorWidget : ModuleWidget {
	AttractorWidget(Attractor* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/attractor.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 15, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 15, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        float X_MIN = 13;

		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN+5, 24)), module, Attractor::CLOCK_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN-6, 43)), module, Attractor::ROC_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN+5, 43)), module, Attractor::SELECT_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN-5, 66)), module, Attractor::A_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN+5, 66)), module, Attractor::B_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN-5, 94)), module, Attractor::C_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN+5, 94)), module, Attractor::D_PARAM));

        float sep = 10;
        /* PARAM INPUTS STACK */
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN-6, 24)), module, Attractor::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN-5, 66 + sep)), module, Attractor::A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN+5, 66 + sep)), module, Attractor::B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN-5, 94 + sep)), module, Attractor::C_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_MIN+5, 94 + sep)), module, Attractor::D_INPUT));

        float JACK_Y = 120.8;
        // OUT L
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8., JACK_Y)), module, Attractor::ATTRACTOR_X));
        // OUT R
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17, JACK_Y)), module, Attractor::ATTRACTOR_Y));
        //addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(X_MIN+40, 48), module, Attractor::PHASE_LIGHT));
	}
};


Model* modelAttractor = createModel<Attractor, AttractorWidget>("Attractor");
