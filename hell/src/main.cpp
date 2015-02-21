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



struct Param {
	enum Type { RADIO, SLIDE };
	const char*	name;
	Type		type;
	float		min;
	float		max;
	bool		is_target;
	const char* options;
};
static const Param params[] = {
	{ "Panning",     Param::SLIDE, -1, 1, true },
	// osc 1
	{ "Mode",        Param::RADIO,  0, 3, false, "Off|Tri|Pul|Sin" },
	{ "Transpose",   Param::SLIDE, -1, 1, true },
	{ "Detune",      Param::SLIDE, -1, 1, true },
	{ "Volume",      Param::SLIDE,  0, 1, true },
	{ "Pulsewidth",  Param::SLIDE,  0, 1, true },
	// osc 2
	{ "Mode",        Param::RADIO,  0, 3, false, "Off|Tri|Pul|Sin" },
	{ "Transpose",   Param::SLIDE, -1, 1, true },
	{ "Detune",      Param::SLIDE, -1, 1, true },
	{ "Volume",      Param::SLIDE,  0, 1, true },
	{ "Pulsewidth",  Param::SLIDE,  0, 1, true },
	// osc 3
	{ "Mode",        Param::RADIO,  0, 3, false, "Off|Tri|Pul|Sin" },
	{ "Transpose",   Param::SLIDE, -1, 1, true },
	{ "Detune",      Param::SLIDE, -1, 1, true },
	{ "Volume",      Param::SLIDE,  0, 1, true },
	{ "Pulsewidth",  Param::SLIDE,  0, 1, true },
	// env 1
	{ "Attack",      Param::SLIDE,  0, 1, true },
	{ "Decay",       Param::SLIDE,  0, 1, true },
	{ "Sustain",     Param::SLIDE,  0, 1, true },
	{ "Release",     Param::SLIDE,  0, 1, true },
	// env 2
	{ "Attack",      Param::SLIDE,  0, 1, true },
	{ "Decay",       Param::SLIDE,  0, 1, true },
	{ "Sustain",     Param::SLIDE,  0, 1, true },
	{ "Release",     Param::SLIDE,  0, 1, true },
	// lfo 1
	{ "Mode",        Param::RADIO,  0, 3, false, "Off|Tri|Pul|Sin" },
	{ "Sync",        Param::RADIO,  0, 1, false, "Off|On" },
	{ "Rate",        Param::SLIDE,  0, 1, true },
	{ "Phase",       Param::SLIDE,  0, 1, true },
	{ "Amplify",     Param::SLIDE,  0, 1, true },
	// lfo 2
	{ "Mode",        Param::RADIO,  0, 3, false, "Off|Tri|Pul|Sin" },
	{ "Sync",        Param::RADIO,  0, 1, false, "Off|On" },
	{ "Rate",        Param::SLIDE,  0, 1, true },
	{ "Phase",       Param::SLIDE,  0, 1, true },
	{ "Amplify",     Param::SLIDE,  0, 1, true },

};


struct Header { int pos; const char* name; };
static const Header headers[] = {
	{  0, "Voice" },
	{  1, "OSC 1" },
	{  6, "OSC 2" },
	{ 11, "OSC 3" },
	{ 16, "ASDR 1" },
	{ 20, "ASDR 2" },
	{ 24, "LFO 1" },
	{ 29, "LFO 2" },
	{}
};


const char* src_params[] = {
	"Velocity",
	"Env 1",
	"Env 2",
	"LFO 1",
	"LFO 2",
};


enum { SRC_VELOCITY, SRC_ENV1, SRC_ENV2, SRC_LFO1, SRC_LFO2 };
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
	{
		{ 1, SRC_LFO1,   3, 1 },	// vibrato
		{ 1, SRC_LFO1,   8, 1 },	// vibrato
		{ 1, SRC_LFO1,  13, 1 },	// vibrato
		{ 1, SRC_LFO2,   0, 0.5 },
	}
};

float*		fvalues = (float*)&patch;
uint32_t* 	ivalues = (uint32_t*)&patch;




class MainWindow : public QFrame { Q_OBJECT
public:
	MainWindow() {


		auto grid = new QGridLayout();
		grid->setColumnStretch(1, 2);
		grid->setColumnMinimumWidth(2, 5);
		grid->setColumnStretch(4, 2);


		QMenu* target_menu = new QMenu();
		connect(target_menu, SIGNAL(triggered(QAction*)), this, SLOT(targetSelected(QAction*)));

		const Header* header = headers;
		const char* header_name;
		int i = 0;
		int row = 0, col = 0;
		for (auto& param : params) {

			// insert header
			if (header->pos == i) {
				if (col > 0) row++;
				col = 0;

				header_name = header->name;
				auto label = new QLabel(header_name);
				label->setProperty("class", "header");
				grid->addWidget(label, row, 0, 1, 5);
				header++;
				row++;
			}

			// param name
			grid->addWidget(new QLabel(param.name), row, col);

			// target menu
			if (param.is_target) {
				auto action = target_menu->addAction((std::string(header_name) + " - " + param.name).c_str());
				action->setProperty("i", i);
			}

			auto layout = new QHBoxLayout();
			grid->addLayout(layout, row, col + 1);

			if (param.type == Param::SLIDE) {
				auto slider = new QSlider(Qt::Horizontal);
				slider->setProperty("i", i);
				slider->setRange(param.min * 100, param.max * 100);
				connect(slider, SIGNAL(valueChanged(int)), this, SLOT(paramSliderMoved(int)));

				auto display = new QLabel("0");
				display->setProperty("class", "display");
				slider->setProperty("display", QVariant::fromValue<QObject*>(display));
				slider->setValue(fvalues[i] * 100);

				layout->addWidget(display);
				layout->addWidget(slider);

			}

			if (param.type == Param::RADIO) {

				auto group = new QButtonGroup();

				const char* opt = param.options;
				for (int b = 0; b <= param.max; b++) {

					const char* p = strchr(opt, '|');
					auto text = std::string(opt, p ? p - opt : strlen(opt));
					opt = p + 1;

					auto button = new QPushButton(text.c_str());
					button->setProperty("class", "radio");
					button->setCheckable(true);
					button->setProperty("ib", i * 256 + b);
					connect(button, SIGNAL(clicked()), this, SLOT(paramButtonClicked()));

					button->setChecked(b == ivalues[i]);
					layout->addWidget(button);
					group->addButton(button);

				}
				layout->addStretch(1);
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


		// patch cords
		auto scroll = new QScrollArea();
		scroll->setWidgetResizable(true);

		grid->addWidget(scroll, 0, 5, -1, 1);
		grid->setColumnStretch(5, 4);

		// source menu
		auto source_menu = new QMenu();
		connect(source_menu, SIGNAL(triggered(QAction*)), this, SLOT(sourceSelected(QAction*)));
		for (int i = 0; i < ARRAY_SIZE(src_params); i++) {
			auto action = source_menu->addAction(src_params[i]);
			action->setProperty("index", i);
		}

		auto layout = new QVBoxLayout();
		for (int i = 0; i < NUM_CORDS; i++) {


			auto power_button = new QPushButton("");
			power_button->setProperty("class", "power");
			power_button->setProperty("slot", i);
			power_button->setCheckable(true);
			bool checked = patch.cords[i].enabled;
			power_button->setChecked(checked);
			power_button->setText(checked ? "On" : "Off");
			connect(power_button, SIGNAL(toggled(bool)), this, SLOT(powerButtonToggled(bool)));


			auto source_button = new QPushButton("");
			source_button->setProperty("class", "source");
			source_button->setProperty("slot", i);
			source_button->setMenu(source_menu);
			connect(source_button, SIGNAL(pressed()), this, SLOT(sourceButtonPressed()));


			auto target_button = new QPushButton("");
			target_button->setProperty("class", "target");
			target_button->setProperty("slot", i);
			target_button->setMenu(target_menu);
			connect(target_button, SIGNAL(pressed()), this, SLOT(targetButtonPressed()));


			auto row = new QHBoxLayout();
			row->addWidget(power_button);
			row->addWidget(source_button);
			row->addWidget(target_button);



			auto slider = new QSlider(Qt::Horizontal);
			slider->setProperty("slot", i);
			slider->setRange(-1 * 100, 1 * 100);
			connect(slider, SIGNAL(valueChanged(int)), this, SLOT(cordSliderMoved(int)));

			auto display = new QLabel("0");
			display->setProperty("class", "display");
			slider->setProperty("display", QVariant::fromValue<QObject*>(display));
			slider->setValue(patch.cords[i].gain * 100);

			row->addWidget(display);
			row->addWidget(slider);


			layout->addLayout(row);

		}
		layout->addStretch(1);

		auto w = new QWidget();
		w->setLayout(layout);
		scroll->setWidget(w);


		// style
		std::ifstream t("style.css");
		std::string str((std::istreambuf_iterator<char>(t)),
						 std::istreambuf_iterator<char>());
		setStyleSheet(str.c_str());


		//resize(600, height());
		setLayout(grid);
		show();
	}

	~MainWindow() {
		SDL_Quit();
		keyboard_kill();
	}

private:
	int target_index;
	int source_index;
	QString target_name;
	QString source_name;

private slots:
	void paramButtonClicked() {
		int ib = sender()->property("ib").toInt();
		ivalues[ib / 256] = ib % 256;
	}
	void paramSliderMoved(int v) {
		fvalues[sender()->property("i").toInt()] = v * 0.01;
		QLabel* display = dynamic_cast<QLabel*>(sender()->property("display").value<QObject*>());
		char str[16];
		sprintf(str, "%d", v);
		display->setText(str);
	}
	void targetSelected(QAction* a) {
		target_index = a->property("i").toInt();
		target_name = a->text();
	}
	void targetButtonPressed() {
		auto button = dynamic_cast<QPushButton*>(sender());
		button->setText(target_name);

		int slot = button->property("slot").toInt();
		patch.cords[slot].trg_index = target_index;
	}
	void sourceSelected(QAction* a) {
		source_index = a->property("i").toInt();
		source_name = a->text();
	}
	void sourceButtonPressed() {
		auto button = dynamic_cast<QPushButton*>(sender());
		button->setText(source_name);
		int slot = button->property("slot").toInt();
		patch.cords[slot].src_index = source_index;
	}
	void powerButtonToggled(bool checked) {
		auto button = dynamic_cast<QPushButton*>(sender());
		button->setText(checked ? "On" : "Off");
		int slot = button->property("slot").toInt();
		patch.cords[slot].enabled = checked;
	}
	void cordSliderMoved(int v) {
		patch.cords[sender()->property("slot").toInt()].gain = v * 0.01;
		QLabel* display = dynamic_cast<QLabel*>(sender()->property("display").value<QObject*>());
		char str[16];
		sprintf(str, "%d", v);
		display->setText(str);
	}
};


#include "../moc/main.moc"


void audio_callback(void* userdata, unsigned char* stream, int len) {
	KeyboardEvent event;
	while (keyboard_poll(&event)) synth_event(event.type, event.a, event.b);
	short* buffer = (short*) stream;
	float f[2];
	for (int i = 0; i < len / 4; i++, buffer += 2) {
		synth_mix(f);
		buffer[0] = f[0] * 6000;
		buffer[1] = f[1] * 6000;
	}
}


int main(int argc, char **argv) {
	synth_init(&patch);
	keyboard_init();
	static SDL_AudioSpec spec = { MIXRATE, AUDIO_S16SYS,
		2, 0, 1024, 0, 0, &audio_callback, NULL
	};
	SDL_OpenAudio(&spec, &spec);
	SDL_PauseAudio(0);

	QApplication app(argc, argv);
	MainWindow win;
	return app.exec();
}
