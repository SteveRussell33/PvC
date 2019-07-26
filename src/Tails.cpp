/*

Tails

*/////////////////////////////////////////////////////////////////////////////



#include "pvc.hpp"


struct Tails : Module {
	enum ParamIds {
		PROB_UI,
		TOSS_MODE,
		TOSS_UI,
		FLIP_UI,
		SET_A_UI,
		SET_B_UI,

		NUM_PARAMS
	};
	enum InputIds {
		SIG_IN,
		PROB_CV,
		TOSS_IN,
		FLIP_IN,
		SET_A_IN,
		SET_B_IN,

		NUM_INPUTS
	};
	enum OutputIds {
		SIG_A_OUT,
		SIG_B_OUT,
		GATE_A_OUT,
		GATE_B_OUT,
		TRIG_A_OUT,
		TRIG_B_OUT,
		
		NUM_OUTPUTS
	};
	enum LightIds {
		A_LED,
		B_LED,

		DIR_LED,
		FLP_LED,

		NUM_LIGHTS
	};

	bool gate = false;
	bool flipMode = false;
	
	dsp::SchmittTrigger tossTrigger;
	dsp::SchmittTrigger flipTrigger;
	dsp::SchmittTrigger resetTrigger;
	dsp::PulseGenerator gatePulse;
	
	Tails() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PROB_UI, 0.0f, 1.0f, 0.5f);
		configParam(TOSS_UI, 0, 1, 0);
		configParam(TOSS_MODE, 0, 1, 0);
		configParam(FLIP_UI, 0, 1, 0);
		configParam(SET_A_UI, 0, 1, 0);
		configParam(SET_B_UI, 0, 1, 0);
	}

	void process(const ProcessArgs &args) override;

	void onReset() override {
		gate = false;
		flipMode = false;
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		
		json_t *gateStateJ = json_boolean(gate);
		json_object_set_new(rootJ, "gateState", gateStateJ);

		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override {
		json_t *gateStateJ = json_object_get(rootJ, "gateState");
		if (gateStateJ)
			gate = json_boolean_value(gateStateJ);
	}
};

void Tails::process(const ProcessArgs &args) {
	flipMode = params[TOSS_MODE].getValue();

	if (tossTrigger.process(inputs[TOSS_IN].getVoltage() + params[TOSS_UI].getValue())) {
		gatePulse.trigger(0.01f);
		if (flipMode)
			gate = (random::uniform() < params[PROB_UI].getValue() + inputs[PROB_CV].getVoltage()) ? !gate : gate; //
		else
			gate = (random::uniform() < (params[PROB_UI].getValue() + inputs[PROB_CV].getVoltage()*0.1f));
	}

	if (flipTrigger.process(inputs[FLIP_IN].getVoltage() + params[FLIP_UI].getValue())) {
		gatePulse.trigger(0.01f);
		gate = !gate;
	}

	if (resetTrigger.process(inputs[SET_A_IN].getVoltage() + params[SET_A_UI].getValue())) {
		gatePulse.trigger(0.01f);
		gate = false;
	}

	if (resetTrigger.process(inputs[SET_B_IN].getVoltage() + params[SET_B_UI].getValue())) {
		gatePulse.trigger(0.01f);
		gate = true;
	}

	outputs[SIG_A_OUT].setVoltage(!gate * inputs[SIG_IN].getVoltage());
	outputs[GATE_A_OUT].setVoltage(!gate * 10.0f);
	outputs[TRIG_A_OUT].setVoltage(!gate * gatePulse.process(1.0/args.sampleRate) * 10.0f);
	outputs[SIG_B_OUT].setVoltage(gate * inputs[SIG_IN].getVoltage());
	outputs[GATE_B_OUT].setVoltage(gate * 10.0f);
	outputs[TRIG_B_OUT].setVoltage(gate * gatePulse.process(1.0/args.sampleRate) * 10.0f);
	lights[A_LED].value = !gate;
	lights[B_LED].value = gate;
	lights[DIR_LED].value = !flipMode;
	lights[FLP_LED].value = flipMode;
}

struct ModeToggle : SVGSwitch {
	ModeToggle() {
		//momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/empty.svg")));
		box.size = Vec(12,6);
	}
};

struct TailsWidget : ModuleWidget {
	TailsWidget(Tails *module);
};

TailsWidget::TailsWidget(Tails *module) {
	setModule(module);
 	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Tails.svg")));
  
	// screws
	// addChild(createWidget<ScrewHead1>(Vec(0, 0)));
	addChild(createWidget<ScrewHead2>(Vec(box.size.x - 30, 0)));
	addChild(createWidget<ScrewHead3>(Vec(15, 365)));
	addChild(createWidget<ScrewHead4>(Vec(box.size.x - 15, 365)));

	addInput(createInput<InPortAud>(Vec(19,22), module, Tails::SIG_IN));

	addParam(createParam<PvCKnob>(Vec(19,64), module, Tails::PROB_UI));
	
	addInput(createInput<InPortCtrl>(Vec(19,88), module, Tails::PROB_CV));
	addInput(createInput<InPortBin>(Vec(19,124), module, Tails::TOSS_IN));
	addParam(createParam<LabelButtonL>(Vec(12,149), module, Tails::TOSS_UI));
	
	addChild(createLight<FourPixLight<OrangeLED>>(Vec(25,165),module, Tails::DIR_LED));
	addChild(createLight<FourPixLight<BlueLED>>(Vec(31,165),module, Tails::FLP_LED));
	addParam(createParam<ModeToggle>(Vec(24,164), module, Tails::TOSS_MODE));
	
	addInput(createInput<InPortBin>(Vec(19,180), module, Tails::FLIP_IN));
	addParam(createParam<LabelButtonL>(Vec(12,205), module, Tails::FLIP_UI));
	
	
	addInput(createInput<InPortBin>(Vec(4,222), module, Tails::SET_A_IN));
	addParam(createParam<LabelButtonS>(Vec(3,247), module, Tails::SET_A_UI));
	
	addChild(createLight<FourPixLight<CyanLED>>(Vec(13,267),module, Tails::A_LED));
	addOutput(createOutput<OutPortVal>(Vec(4,276), module, Tails::SIG_A_OUT));
	addOutput(createOutput<OutPortBin>(Vec(4,312), module, Tails::GATE_A_OUT));
	addOutput(createOutput<OutPortBin>(Vec(4,336), module, Tails::TRIG_A_OUT));

	addInput(createInput<InPortBin>(Vec(34,222), module, Tails::SET_B_IN));
	addParam(createParam<LabelButtonS>(Vec(33,247), module, Tails::SET_B_UI));
	
	addChild(createLight<FourPixLight<PurpleLED>>(Vec(43,267),module, Tails::B_LED));
	addOutput(createOutput<OutPortVal>(Vec(34,276), module, Tails::SIG_B_OUT));
	addOutput(createOutput<OutPortBin>(Vec(34,312), module, Tails::GATE_B_OUT));
	addOutput(createOutput<OutPortBin>(Vec(34,336), module, Tails::TRIG_B_OUT));
}
Model *modelTails = createModel<Tails, TailsWidget>("Tails");

