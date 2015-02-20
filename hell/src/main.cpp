#include <stdio.h>
#include <SDL/SDL.h>

#include <string>
#include <fstream>
#include <streambuf>

#include <QtGui>


extern "C" {
#include "keyboard.h"
#include "synth.h"
}


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))


SynthPatch patch = {
	{
		0,
		{
			{ OSC_PULSE, 0,  0.0, 0.7, 0.5 },
			{ OSC_PULSE, 0, -0.2, 0.7, 0.5 },
			{ OSC_PULSE, 0,  0.2, 0.7, 0.5 },
		}, {
			{ 1, 1, 0.5, 0.1 },
			{ 1, 1, 0.5, 0.1 },
		}, {
			{ OSC_SINE, 1, 0.8, 0, 0.25 },
			{ OSC_SINE, 0, 0.5, 0, 0.5 },
		}
	},
	0,
	{
		{ SRC_LFO1,   3, 1 },	// vibrato
		{ SRC_LFO1,   8, 1 },	// vibrato
		{ SRC_LFO1,  13, 1 },	// vibrato
		{ SRC_LFO2,   0, 0.5 },
	}
};

float*		fvalues = (float*)&patch;
uint32_t* 	ivalues = (uint32_t*)&patch;




void audio_callback(void* userdata, unsigned char* stream, int len) {
	KeyboardEvent event;
	while (keyboard_poll(&event)) synth_event(event.type, event.a, event.b);
	short* buffer = (short*) stream;
	float f[2];
	for (int i = 0; i < len / 4; i++, buffer += 2) {
		synth_mix(f);
		buffer[0] = f[0] * 4000;
		buffer[1] = f[1] * 4000;
	}
}



struct Param {
	enum Type { RADIO, SLIDE };
	const char*	name;
	Type		type;
	float		min;
	float		max;
	const char* options;
};


static const Param params[] = {
	{ "Panning",     Param::SLIDE, -1, 1 },
	// osc 1
	{ "Mode",        Param::RADIO,  0, 3, "Off|Tri|Pul|Sin" },
	{ "Transpose",   Param::SLIDE, -1, 1 },
	{ "Detune",      Param::SLIDE, -1, 1 },
	{ "Volume",      Param::SLIDE,  0, 1 },
	{ "Pulsewidth",  Param::SLIDE,  0, 1 },
	// osc 2
	{ "Mode",        Param::RADIO,  0, 3, "Off|Tri|Pul|Sin" },
	{ "Transpose",   Param::SLIDE, -1, 1 },
	{ "Detune",      Param::SLIDE, -1, 1 },
	{ "Volume",      Param::SLIDE,  0, 1 },
	{ "Pulsewidth",  Param::SLIDE,  0, 1 },
	// osc 3
	{ "Mode",        Param::RADIO,  0, 3, "Off|Tri|Pul|Sin" },
	{ "Transpose",   Param::SLIDE, -1, 1 },
	{ "Detune",      Param::SLIDE, -1, 1 },
	{ "Volume",      Param::SLIDE,  0, 1 },
	{ "Pulsewidth",  Param::SLIDE,  0, 1 },
	// env 1
	{ "Attack",      Param::SLIDE,  0, 1 },
	{ "Decay",       Param::SLIDE,  0, 1 },
	{ "Sustain",     Param::SLIDE,  0, 1 },
	{ "Release",     Param::SLIDE,  0, 1 },
	// env 2
	{ "Attack",      Param::SLIDE,  0, 1 },
	{ "Decay",       Param::SLIDE,  0, 1 },
	{ "Sustain",     Param::SLIDE,  0, 1 },
	{ "Release",     Param::SLIDE,  0, 1 },
	// lfo 1
	{ "Mode",        Param::RADIO,  0, 3, "Off|Tri|Pul|Sin" },
	{ "Sync",        Param::RADIO,  0, 1, "Off|On" },
	{ "Rate",        Param::SLIDE,  0, 1 },
	{ "Phase",       Param::SLIDE,  0, 1 },
	{ "Amplify",     Param::SLIDE,  0, 1 },
	// lfo 2
	{ "Mode",        Param::RADIO,  0, 3, "Off|Tri|Pul|Sin" },
	{ "Sync",        Param::RADIO,  0, 1, "Off|On" },
	{ "Rate",        Param::SLIDE,  0, 1 },
	{ "Phase",       Param::SLIDE,  0, 1 },
	{ "Amplify",     Param::SLIDE,  0, 1 },

};


struct Header {
	int			pos;
	const char*	name;
};


static const Header headers[] = {
	{  1, "OSC 1" },
	{  6, "OSC 2" },
	{ 11, "OSC 3" },
	{ 16, "ASDR 1" },
	{ 20, "ASDR 2" },
	{ 24, "LFO 1" },
	{ 29, "LFO 2" },
	{}
};



class MainWindow : public QFrame {
	Q_OBJECT

public:
	MainWindow() {

		synth_init(&patch);
		keyboard_init();
		static SDL_AudioSpec spec = { MIXRATE, AUDIO_S16SYS,
			2, 0, 1024, 0, 0, &audio_callback, NULL
		};
		SDL_OpenAudio(&spec, &spec);
		SDL_PauseAudio(0);



		auto grid = new QGridLayout();
		grid->setColumnStretch(1, 1);
		grid->setColumnMinimumWidth(3, 20);
		grid->setColumnStretch(4, 1);



		int i = 0, h = 0;
		int row = 0, col = 0;



		for (auto& param : params) {

			// insert header
			if (headers[h].pos == i) {
				if (col > 0) row++;
				col = 0;

				auto text = std::string("<b>") + headers[h].name + "</b>";
				auto label = new QLabel(text.c_str());
				//grid->setRowMinimumHeight(row, 30);
				//label->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
				label->setAlignment(Qt::AlignHCenter);

				grid->addWidget(label, row, 0, 1, -1);
				h++;
				row++;
			}


			// param name
			grid->addWidget(new QLabel(param.name), row, col);


			if (param.type == Param::SLIDE) {
				auto slider = new QSlider(Qt::Horizontal);
				slider->setProperty("i", i);
				connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderMoved(int)));
				slider->setRange(param.min * 1000, param.max * 1000);
				slider->setValue(fvalues[i] * 1000);

				grid->addWidget(slider, row, col + 1);
			}

			if (param.type == Param::RADIO) {

				auto group = new QButtonGroup();
				auto button_layout = new QHBoxLayout();
				grid->addLayout(button_layout, row, col + 1);

				const char* opt = param.options;
				for (int b = 0; b <= param.max; b++) {

					const char* p = strchr(opt, '|');
					auto text = std::string(opt, p ? p - opt : strlen(opt));
					opt = p + 1;

					auto button = new QRadioButton(text.c_str());
					button->setProperty("ib", i * 256 + b);
					connect(button, SIGNAL(clicked()), this, SLOT(buttonClicked()));

					button->setChecked(b == ivalues[i]);
					button_layout->addWidget(button);
					group->addButton(button);

					buttons[i][b] = button;

				}
				button_layout->addStretch(1);
			}

			if (col == 0) col = 3;
			else {
				col = 0;
				row++;
			}
			i++;
		}
		if (col > 0) row ++;
		grid->setRowStretch(row, 1);
		grid->setRowMinimumHeight(row, 0);


		setLayout(grid);

		resize(600, height());


		// style
		std::ifstream t("style.css");
		std::string str((std::istreambuf_iterator<char>(t)),
						 std::istreambuf_iterator<char>());
		setStyleSheet(str.c_str());


		show();
	}

	~MainWindow() {
		SDL_Quit();
		keyboard_kill();
	}

private slots:
	void buttonClicked() {
		int ib = sender()->property("ib").toInt();
		ivalues[ib / 256] = ib % 256;
		//printf("button %02x\n", ib);
	}
	void sliderMoved(int v) {
		int i = sender()->property("i").toInt();
		fvalues[i] = v * 0.001;
		//printf("slider %x %d\n", i, v);
	}


private:
	QRadioButton* buttons[ARRAY_SIZE(params)][9];

};

#include "../moc/main.moc"



int main(int argc, char **argv) {

	QApplication app(argc, argv);
	MainWindow win;

	return app.exec();
}
