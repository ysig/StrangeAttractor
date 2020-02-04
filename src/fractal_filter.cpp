#include "plugin.hpp"
#include <math.h>


struct FractalFilter : Module {
	enum ParamIds {
		P_PARAM,
		D_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SIGNAL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SIGNAL_OUTPUT,
		NUM_OUTPUTS
	};

    /*enum LightIds {
		ENUMS(PHASE_LIGHT, 3),
		NUM_LIGHTS
	};*/

    double y=0;
    long ind = 0;

	FractalFilter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);//, NUM_LIGHTS);
		configParam(P_PARAM, 0.f, 1.f, 0.2f, "Amount of filtering.");
		configParam(D_PARAM, 0.f, 1.f, 0.1f, "Fractal Dimension");
	}

	void process(const ProcessArgs& args) override {
		// https://www.sciencedirect.com/science/article/pii/000527369090427P
        if(outputs[SIGNAL_INPUT].isConnected() && outputs[SIGNAL_OUTPUT].isConnected()){
            float D = params[D_PARAM].getValue();
            float p = params[P_PARAM].getValue();
            double w = 1/(1 + pow(p, 1.5 - D));
	
            float x = inputs[SIGNAL_INPUT].getVoltage();
            y = w*y + (1-w)*x;
		    // Outputs
		    outputs[SIGNAL_OUTPUT].setVoltage(y);
        }else{
        	y = 0;
        }
		/*if (lightDivider.process()) {
			float lightValue = simd::sin(2 * M_PI * phase);
			lights[PHASE_LIGHT + 0].setSmoothBrightness(-lightValue, args.sampleTime * lightDivider.getDivision());
			lights[PHASE_LIGHT + 1].setSmoothBrightness(lightValue, args.sampleTime * lightDivider.getDivision());
			lights[PHASE_LIGHT + 2].setBrightness(0.f);
		}*/
	}
};


struct FractalFilterWidget : ModuleWidget {
	FractalFilterWidget(FractalFilter* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/frac.svg")));//FractalFilter.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH - 15, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 15, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        float X_MIN = 13;
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN-5, 66)), module, FractalFilter::P_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(X_MIN+5, 66)), module, FractalFilter::D_PARAM));

        float JACK_Y = 120.8;
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8., JACK_Y)), module, FractalFilter::SIGNAL_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17, JACK_Y)), module, FractalFilter::SIGNAL_OUTPUT));
	}
};


Model* modelFractalFilter = createModel<FractalFilter, FractalFilterWidget>("FractalFilter");
