#include <QtCore/qglobal.h>
#include <QFileDialog>
#include <QColorDialog> 
#include <CGAL/Qt/manipulatedCameraFrame.h>
#include <CGAL/Qt/manipulatedFrame.h>

#include <fstream>


#include "Messages_interface.h"
#include "Scene_points_with_normal_item.h"
#include "Item_classification_base.h"
#include "Point_set_item_classification.h"
#include "Cluster_classification.h"
#include "Scene_surface_mesh_item.h"
#include "Surface_mesh_item_classification.h"
#include "Scene_polylines_item.h"
#include "Scene_polygon_soup_item.h"

#include "CGAL/Three/Scene_interface.h" //#include <CGAL/Three/Scene_interface.h>
#include <CGAL/Three/Polyhedron_demo_plugin_helper.h>
#include "CGAL/Three/Three.h" //#include <CGAL/Three/Three.h>

#include <CGAL/Random.h>
#include <CGAL/Real_timer.h>

#include <QMultipleInputDialog.h>

#include "ui_Classification_widget.h"
#include "ui_Classification_advanced_widget.h"

#include <QAction>
#include <QMainWindow>
#include <QApplication>
#include <QCheckBox>
#include <QRadioButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>

#include <map>

#include <boost/graph/adjacency_list.hpp>
#include <CGAL/boost/graph/split_graph_into_polylines.h>

#define CGAL_CLASSIFICATION_ETHZ_ID "Random Forest (ETHZ)"
#define CGAL_CLASSIFICATION_TENSORFLOW_ID "Neural Network (TensorFlow)"
#define CGAL_CLASSIFICATION_OPENCV_ID "Random Forest (OpenCV)"
#define CGAL_CLASSIFICATION_SOWF_ID "Sum of Weighted Features"

using namespace CGAL::Three;
class Polyhedron_demo_classification_plugin :
	public QObject,
	public Polyhedron_demo_plugin_helper
{
	Q_OBJECT
		Q_INTERFACES(CGAL::Three::Polyhedron_demo_plugin_interface)
		Q_PLUGIN_METADATA(IID "com.geometryfactory.PolyhedronDemo.PluginInterface/1.0")

		struct LabelButton
	{
		QPushButton* color_button;
		QMenu* menu;
		char shortcut;

		QColor color;

		QLabel* label2;
		QComboBox* effect;

		LabelButton(QWidget* parent,
			const char* name,
			const QColor& color,
			const char shortcut)
			: shortcut(shortcut), color(color)
		{
			color_button = new QPushButton(tr("%1 (%2)").arg(name).arg((char)(std::toupper(shortcut))), parent);

			menu = new QMenu("Label Menu", color_button);

			QColor text_color(255, 255, 255);
			if (color.red() * 0.299 + color.green() * 0.587 + color.blue() * 0.114 > 128)
				text_color = QColor(0, 0, 0);

			QString s("QPushButton { font-weight: bold; background: #"
				+ QString(color.red() < 16 ? "0" : "") + QString::number(color.red(), 16)
				+ QString(color.green() < 16 ? "0" : "") + QString::number(color.green(), 16)
				+ QString(color.blue() < 16 ? "0" : "") + QString::number(color.blue(), 16)
				+ "; color: #"
				+ QString(text_color.red() < 16 ? "0" : "") + QString::number(text_color.red(), 16)
				+ QString(text_color.green() < 16 ? "0" : "") + QString::number(text_color.green(), 16)
				+ QString(text_color.blue() < 16 ? "0" : "") + QString::number(text_color.blue(), 16)
				+ "; }");

			color_button->setStyleSheet(s);
			color_button->setMenu(menu);


			label2 = new QLabel(name, parent);
			effect = new QComboBox;
			effect->addItem("Penalized");
			effect->addItem("Neutral");
			effect->addItem("Favored");
		}
		~LabelButton()
		{
		}
		void change_color(const QColor& color)
		{
			this->color = color;
			QColor text_color(255, 255, 255);
			if (color.red() * 0.299 + color.green() * 0.587 + color.blue() * 0.114 > 128)
				text_color = QColor(0, 0, 0);
			QString s("QPushButton { font-weight: bold; background: #"
				+ QString(color.red() < 16 ? "0" : "") + QString::number(color.red(), 16)
				+ QString(color.green() < 16 ? "0" : "") + QString::number(color.green(), 16)
				+ QString(color.blue() < 16 ? "0" : "") + QString::number(color.blue(), 16)
				+ "; color: #"
				+ QString(text_color.red() < 16 ? "0" : "") + QString::number(text_color.red(), 16)
				+ QString(text_color.green() < 16 ? "0" : "") + QString::number(text_color.green(), 16)
				+ QString(text_color.blue() < 16 ? "0" : "") + QString::number(text_color.blue(), 16)
				+ "; }");

			color_button->setStyleSheet(s);
			color_button->update();
		}

	};

public:
	bool applicable(QAction*) const {
		return
			qobject_cast<Scene_points_with_normal_item*>(scene->item(scene->mainSelectionIndex()))
			|| qobject_cast<Scene_surface_mesh_item*>(scene->item(scene->mainSelectionIndex()));
	}
	void print_message(QString message) { CGAL::Three::Three::information(message); }
	QList<QAction*> actions() const { return QList<QAction*>() << actionClassification; }

	using Polyhedron_demo_plugin_helper::init;
	void init(QMainWindow* mainWindow, CGAL::Three::Scene_interface* scene_interface, Messages_interface* m) {

		mw = mainWindow;
		scene = scene_interface;
		messages = m;
		//***********************Weixiao Update menu name*******************************//
		//actionClassification = new QAction(tr("Classification"), mw);
		actionClassification = new QAction(tr("3D Annotation"), mw);
		//********************************************************************//
		connect(actionClassification, SIGNAL(triggered()), this, SLOT(classification_action()));
		//***********************Weixiao Update menu name*******************************//
		//dock_widget = new QDockWidget("Classification", mw);
		dock_widget = new QDockWidget("3D Annotation", mw);
		//********************************************************************//
		dock_widget->setVisible(false);

		//***********************Weixiao Update menu name*******************************//
		//dock_widget_adv = new QDockWidget("Classification (Advanced)", mw);
		dock_widget_adv = new QDockWidget("3D Annotation (Advanced)", mw);
		//***********************Weixiao Update menu name*********************//
		dock_widget_adv->setDisabled(true);
		dock_widget_adv->setVisible(false);
		//********************************************************************//
		//********************************************************************//

		label_button = new QPushButton(QIcon(QString(":/cgal/icons/plus")), "", dock_widget);
		//***********************Weixiao Update menu name*********************//
		label_button->setDisabled(true);
		label_button->setVisible(false);
		//********************************************************************//
		QMenu* label_menu = new QMenu("Label Menu", label_button);
		label_button->setMenu(label_menu);
		QAction* add_new_label = label_menu->addAction("Add new label(s)");
		connect(add_new_label, SIGNAL(triggered()), this,
			SLOT(on_add_new_label_clicked()));

		label_menu->addSeparator();

		QAction* use_config_building = label_menu->addAction("Use configuration ground/vegetation/building");
		//***********************Weixiao Update menu name*********************//
		use_config_building->setDisabled(true);
		use_config_building->setVisible(false);
		//********************************************************************//
		connect(use_config_building, SIGNAL(triggered()), this,
			SLOT(on_use_config_building_clicked()));
		QAction* use_config_roof = label_menu->addAction("Use configuration ground/vegetation/roof/facade");
		//***********************Weixiao Update menu name*********************//
		use_config_roof->setDisabled(true);
		use_config_roof->setVisible(false);
		//********************************************************************//
		connect(use_config_roof, SIGNAL(triggered()), this,
			SLOT(on_use_config_roof_clicked()));
		QAction* use_config_las = label_menu->addAction("Use LAS standard configuration");
		//***********************Weixiao Update menu name*********************//
		use_config_las->setDisabled(true);
		use_config_las->setVisible(false);
		//********************************************************************//
		connect(use_config_las, SIGNAL(triggered()), this,
			SLOT(on_use_las_config_clicked()));

		label_menu->addSeparator();


		QAction* generate = label_menu->addAction("Create one point set item per label");
		//***********************Weixiao Update menu name*********************//
		generate->setDisabled(true);
		generate->setVisible(false);
		//********************************************************************//
		connect(generate, SIGNAL(triggered()), this,
			SLOT(on_generate_items_button_clicked()));

		label_menu->addSeparator();

		QAction* clear_labels = label_menu->addAction("Clear labels");
		//***********************Weixiao Update menu name*********************//
		clear_labels->setDisabled(true);
		clear_labels->setVisible(false);
		//********************************************************************//
		connect(clear_labels, SIGNAL(triggered()), this,
			SLOT(on_clear_labels_clicked()));

		ui_widget.setupUi(dock_widget);
		ui_widget_adv.setupUi(dock_widget_adv);
		addDockWidget(dock_widget);
		addDockWidget(dock_widget_adv);

		color_att = QColor(75, 75, 77);

		QAction* compute_features = ui_widget.features_menu->addAction("Compute features...");
		connect(compute_features, SIGNAL(triggered()), this,
			SLOT(on_compute_features_button_clicked()));

		action_statistics = ui_widget.features_menu->addAction("Show feature statistics");
		connect(action_statistics, SIGNAL(triggered()), this,
			SLOT(on_statistics_clicked()));

		action_reset_local = ui_widget.training_menu->addAction("Reset training set of selection");
		connect(action_reset_local, SIGNAL(triggered()), this,
			SLOT(on_reset_training_set_of_selection_clicked()));

		action_reset = ui_widget.training_menu->addAction("Reset all training sets");
		connect(action_reset, SIGNAL(triggered()), this,
			SLOT(on_reset_training_sets_clicked()));

		action_random_region = ui_widget.training_menu->addAction("Select random region");
		action_random_region->setShortcut(Qt::SHIFT | Qt::Key_S);
		connect(action_random_region, SIGNAL(triggered()), this,
			SLOT(on_select_random_region_clicked()));

		action_validate = ui_widget.training_menu->addAction("Validate labels of current selection as training sets");
		connect(action_validate, SIGNAL(triggered()), this,
			SLOT(on_validate_selection_clicked()));

		classifier = ui_widget.classifier_menu->addSection(CGAL_CLASSIFICATION_ETHZ_ID);

		action_train = ui_widget.classifier_menu->addAction("Train...");
		action_train->setShortcut(Qt::SHIFT | Qt::Key_T);
		connect(action_train, SIGNAL(triggered()), this,
			SLOT(on_train_clicked()));

		ui_widget.classifier_menu->addSeparator();

		action_run = ui_widget.classifier_menu->addAction("Classify");
		connect(action_run, SIGNAL(triggered()), this,
			SLOT(on_run_button_clicked()));

		action_run_smoothed = ui_widget.classifier_menu->addAction("Classify with local smoothing...");
		connect(action_run_smoothed, SIGNAL(triggered()), this,
			SLOT(on_run_smoothed_button_clicked()));

		action_run_graphcut = ui_widget.classifier_menu->addAction("Classify with Graph Cut...");
		connect(action_run_graphcut, SIGNAL(triggered()), this,
			SLOT(on_run_graphcut_button_clicked()));

		ui_widget.classifier_menu->addSeparator();

		action_save_config = ui_widget.classifier_menu->addAction("Save current configuration...");
		action_load_config = ui_widget.classifier_menu->addAction("Load configuration...");
		connect(action_save_config, SIGNAL(triggered()), this,
			SLOT(on_save_config_button_clicked()));
		connect(action_load_config, SIGNAL(triggered()), this,
			SLOT(on_load_config_button_clicked()));

		ui_widget.classifier_menu->addSeparator();

		QAction* switch_classifier = ui_widget.classifier_menu->addAction("Switch to another classifier...");
		connect(switch_classifier, SIGNAL(triggered()), this,
			SLOT(on_switch_classifier_clicked()));

		//connect(ui_widget.minDisplay, SIGNAL(released()), this,
		//	SLOT(on_min_display_button_clicked()));
		//connect(ui_widget.maxDisplay, SIGNAL(released()), this,
		//	SLOT(on_max_display_button_clicked()));

		connect(ui_widget_adv.selected_feature, SIGNAL(currentIndexChanged(int)), this,
			SLOT(on_selected_feature_changed(int)));
		connect(ui_widget_adv.feature_weight, SIGNAL(valueChanged(int)), this,
			SLOT(on_feature_weight_changed(int)));

		connect(ui_widget.help, SIGNAL(clicked()), this,
			SLOT(on_help_clicked()));
		connect(ui_widget.close, SIGNAL(clicked()), this,
			SLOT(ask_for_closing()));

		//***********************Ziqian && Weixiao*******************************//
		connect(ui_widget.display, SIGNAL(currentIndexChanged(int)), this,
			SLOT(on_display_button_clicked_with_probability(int)));

		connect(ui_widget.ProbSpin, SIGNAL(valueChanged(int)), ui_widget.ProbSlider, SLOT(setValue(int)));
		connect(ui_widget.ProbSlider, SIGNAL(valueChanged(int)), ui_widget.ProbSpin, SLOT(setValue(int)));

		connect(ui_widget.ProbSpin, SIGNAL(valueChanged(int)), this, SLOT(on_probability_threshold_changed(int)));
		//connect(ui_widget.ProbSlider, SIGNAL(valueChanged(int)), this, SLOT(on_probability_threshold_changed(int)));

		connect(ui_widget.ProbSwitcher, SIGNAL(currentIndexChanged(int)), this, SLOT(on_probability_switcher_changed(int)));
		//********************************************************************//

		//*******************************Weixiao **************************************//
		//connect(ui_widget.lineEdit, SIGNAL(triggered()), this, SLOT(show_total_lables(int))); //Total
		//connect(ui_widget.lineEdit_2, SIGNAL(on_add_selection_to_training_set_clicked()), this, SLOT(get_unlabelled_number_facets())); //Finished
		//connect(ui_widget.progressBar, SIGNAL(currentIndexChanged(int)), this, SLOT(show_progress_bar(int)));
		//********************************************************************//


		QObject* scene_obj = dynamic_cast<QObject*>(scene_interface);
		if (scene_obj)
		{
			connect(scene_obj, SIGNAL(itemAboutToBeDestroyed(CGAL::Three::Scene_item*)), this,
				SLOT(item_about_to_be_destroyed(CGAL::Three::Scene_item*)));

			connect(scene_obj, SIGNAL(itemIndexSelected(int)), this,
				SLOT(update_plugin(int)));
		}
	}
	virtual void closure()
	{
		dock_widget->hide();
		close_classification();
	}


public Q_SLOTS:

	void item_about_to_be_destroyed(CGAL::Three::Scene_item* scene_item) {
		Item_map::iterator it = item_map.find(scene_item);
		if (it != item_map.end())
		{
			Item_classification_base* classif = it->second;
			item_map.erase(it); // first erase from map, because scene->erase will cause a call to this function
			delete classif;
		}
	}

	void classification_action()
	{
		dock_widget->show();
		dock_widget->raise();
		if (Scene_points_with_normal_item * points_item
			= qobject_cast<Scene_points_with_normal_item*>(scene->item(scene->mainSelectionIndex())))
		{
			create_from_item(points_item);
			QAction* ps_selection = mw->findChild<QAction*>("actionPointSetSelection");
			if (ps_selection)
				ps_selection->trigger();
			else
				print_message("Warning: can't find Point Set Selection plugin");
		}
		else if (Scene_surface_mesh_item * mesh_item
			= qobject_cast<Scene_surface_mesh_item*>(scene->item(scene->mainSelectionIndex())))
		{
			create_from_item(mesh_item);
			QAction* sm_selection = mw->findChild<QAction*>("actionSelection");
			if (sm_selection)
				sm_selection->trigger();
			else
				print_message("Warning: can't find Surface Mesh Selection plugin");

			//********************Weixiao Update************************//
			CGAL::Three::Three::SetdefaultSurfaceMeshRenderingMode(TextureModePlusFlatEdges);
			mesh_item->setRenderingMode(TextureModePlusFlatEdges);
			CGAL::Three::Three::information(QString("Reset the default rendering mode to TextureModePlusFlatEdges"));
			//**********************************************************//
		}

		on_help_clicked();
	}

	void ask_for_closing()
	{
		QMessageBox oknotok;
		oknotok.setWindowTitle("Closing classification");
		oknotok.setText("All computed data structures will be discarded.\nColored display will be reinitialized.\nLabels and training information will remain in the classified items.\n\nAre you sure you want to close?");
		oknotok.setStandardButtons(QMessageBox::Yes);
		oknotok.addButton(QMessageBox::No);
		oknotok.setDefaultButton(QMessageBox::Yes);

		if (oknotok.exec() == QMessageBox::Yes)
			close_classification();
	}

	void close_classification()
	{
		for (Item_map::iterator it = item_map.begin(); it != item_map.end(); ++it)
		{
			Item_classification_base* classif = it->second;
			item_changed(classif->item());
			delete classif;
		}
		item_map.clear();
		dock_widget->hide();
	}

	void item_changed(Scene_item* item)
	{
		scene->itemChanged(item);
		item->invalidateOpenGLBuffers();
	}

	void update_plugin(int i)
	{
		if (dock_widget->isVisible())
			update_plugin_from_item(get_classification(scene->item(i)));
	}

	void disable_everything()
	{
		ui_widget.features_menu->setEnabled(false);
		ui_widget.training_menu->setEnabled(false);
		ui_widget.classifier_menu->setEnabled(false);
		ui_widget.view->setEnabled(false);
		ui_widget.frame->setEnabled(false);
	}

	void enable_computation()
	{
		ui_widget.features_menu->setEnabled(false);//true
		ui_widget.training_menu->setEnabled(false);//true
		ui_widget.classifier_menu->setEnabled(false);
		action_statistics->setEnabled(false);
		action_train->setEnabled(false);
		action_reset_local->setEnabled(false);//true
		action_reset->setEnabled(false);//true
		action_random_region->setEnabled(false);//true
		action_validate->setEnabled(false);//true
		action_save_config->setEnabled(false);
		action_load_config->setEnabled(false);
		action_run->setEnabled(false);
		action_run_smoothed->setEnabled(false);
		action_run_graphcut->setEnabled(false);
		ui_widget.view->setEnabled(true);
		ui_widget.frame->setEnabled(true);
	}

	void enable_classif()
	{
		ui_widget.features_menu->setEnabled(false);//true
		ui_widget.training_menu->setEnabled(false);//true
		ui_widget.classifier_menu->setEnabled(false);//true
		action_statistics->setEnabled(false);//true
		action_train->setEnabled(false);//true
		action_reset_local->setEnabled(false);//true
		action_reset->setEnabled(false);//true
		action_random_region->setEnabled(false);//true
		action_validate->setEnabled(false);//true
		action_save_config->setEnabled(false);//true
		action_load_config->setEnabled(false);//true
		action_run->setEnabled(false);//true
		action_run_smoothed->setEnabled(false);//true
		action_run_graphcut->setEnabled(false);//true
		ui_widget.frame->setEnabled(true);
	}

	void update_plugin_from_item(Item_classification_base* classif)
	{
		disable_everything();
		if (classif != NULL)
		{
			enable_computation();
			// Clear class labels
			for (std::size_t i = 0; i < label_buttons.size(); ++i)
			{
				ui_widget.labelGrid->removeWidget(label_buttons[i].color_button);
				label_buttons[i].color_button->deleteLater();
				label_buttons[i].menu->deleteLater();
				ui_widget_adv.gridLayout->removeWidget(label_buttons[i].label2);
				delete label_buttons[i].label2;
				ui_widget_adv.gridLayout->removeWidget(label_buttons[i].effect);
				delete label_buttons[i].effect;
			}
			label_buttons.clear();

			// Add labels
			for (std::size_t i = 0; i < classif->number_of_labels(); ++i)
				add_new_label(LabelButton(dock_widget,
					classif->label(i)->name().c_str(),
					classif->label_color(i),
					get_shortcut(i, classif->label(i)->name().c_str())));
			add_label_button();

			// Enabled classif if features computed
			if (classif->features_computed())
				enable_classif();

			int index = ui_widget.display->currentIndex();
			ui_widget.display->clear();
			ui_widget.display->addItem("Real colors");
			ui_widget.display->addItem("Editing");
			ui_widget.display->addItem("Unlabelled");
			ui_widget.display->setCurrentIndex(1);
			ui_widget_adv.selected_feature->clear();
			classif->fill_display_combo_box(ui_widget.display, ui_widget_adv.selected_feature);

			//******************Weixiao**************************//
			if (!classif->can_show_probability())
			{
				int total_facet_num = classif->get_total_number_facets();
				ui_widget.lineEdit->setText(QString::number(total_facet_num));
				int unlabelled_num = classif->get_unlabelled_number_facets();
				ui_widget.lineEdit_2->setText(QString::number(unlabelled_num));
				float labeling_progress = 1.0f - float(unlabelled_num) / float(total_facet_num);
				labeling_progress = total_facet_num == unlabelled_num ? 0.0f : labeling_progress * 100.0f;
				ui_widget.progressBar->setValue(labeling_progress);
			}
			//**************************************************************//

			ui_widget_adv.selected_feature->setCurrentIndex(0);
			//********************Weixiao Update************************//
			Scene_polyhedron_selection_item* selection_item
				= qobject_cast<Scene_polyhedron_selection_item*>(scene->item(scene->mainSelectionIndex()));
			if (selection_item && selection_item->polyhedron_item()->label_selection_combox_tmp != NULL)
			{
				selection_item->polyhedron_item()->fill_classes_combo_box(selection_item->polyhedron_item()->label_selection_combox_tmp);
				selection_item->polyhedron_item()->update_labels_for_selection();
			}
			//**********************************************************//
		}
	}

	Item_classification_base* get_classification(Scene_item* item = NULL)
	{
		if (!item)
			item = scene->item(scene->mainSelectionIndex());

		if (!item)
			return NULL;

		Scene_polyhedron_selection_item* selection_item
			= qobject_cast<Scene_polyhedron_selection_item*>(item);
		if (selection_item)
			item = selection_item->polyhedron_item();

		Item_map::iterator it = item_map.find(item);

		if (it != item_map.end())
		{
			if (selection_item)
				dynamic_cast<Surface_mesh_item_classification*>(it->second)->set_selection_item(selection_item);
			return it->second;
		}

		return NULL;
	}

	Item_classification_base* create_from_item(Scene_points_with_normal_item* points_item)
	{
		if (item_map.find(points_item) != item_map.end())
			return item_map[points_item];

		bool use_clusters = false;

		if (points_item->point_set()->has_property_map<int>("shape"))
		{
			QMessageBox::StandardButton reply
				= QMessageBox::question(NULL, "Point Set Classification",
					"This point set is divided in clusters. Do you want to classify clusters instead of points?",
					QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

			use_clusters = (reply == QMessageBox::Yes);
		}

		QApplication::setOverrideCursor(Qt::WaitCursor);

		Item_classification_base* classif;
		if (use_clusters)
			classif = new Cluster_classification(points_item);
		else
			classif = new Point_set_item_classification(points_item);

		item_map.insert(std::make_pair(points_item, classif));

		QApplication::restoreOverrideCursor();
		update_plugin_from_item(classif);
		return classif;
	}

	Item_classification_base* create_from_item(Scene_surface_mesh_item* mesh_item)
	{
		if (item_map.find(mesh_item) != item_map.end())
			return item_map[mesh_item];

		QApplication::setOverrideCursor(Qt::WaitCursor);
		Item_classification_base* classif
			= new Surface_mesh_item_classification(mesh_item);
		item_map.insert(std::make_pair(mesh_item, classif));
		QApplication::restoreOverrideCursor();
		update_plugin_from_item(classif);

		/************Weixiao Update************/
		if (!classif->can_show_probability())
			print_message("Input data do not have probability or with invalid probability, \
			so the probability slider will not work. The good news is the progress bar will work perfectly!!!");
		else
			print_message("Input data have probability and predict labels, so the labeling progress bar will not work. \
			The good news is the probability slider will work perfectly!!!");
		/**************************************/

		return classif;
	}

	int get_classifier()
	{
		if (classifier->text() == QString(CGAL_CLASSIFICATION_ETHZ_ID))
			return 1;
		if (classifier->text() == QString(CGAL_CLASSIFICATION_TENSORFLOW_ID))
			return 3;
		if (classifier->text() == QString(CGAL_CLASSIFICATION_OPENCV_ID))
			return 2;
		if (classifier->text() == QString(CGAL_CLASSIFICATION_SOWF_ID))
			return 0;

		std::cerr << "Error: unknown classifier" << std::endl;
		return -1;
	}

	void run(Item_classification_base* classif, int method,
		std::size_t subdivisions = 1,
		double smoothing = 0.5)
	{
		classif->run(method, get_classifier(), subdivisions, smoothing);
	}

	void on_help_clicked()
	{
		QMessageBox::information(dock_widget, QString("3D Annotator"),
			//QString("3D Annotator\n"
			//	"\n"
			//	"Welcome to 3D Annotator! Please read carefully this notice\n"
			//	"before using the plugin.\n"
			//	"\n"
			//	"[QUICK INTRODUCTION]\n"
			//	"\n"
			//	"In order to classify, you need to perform the following steps:\n"
			//	"\n"
			//	"1. Compute the features\n"
			//	"2. Set up the labels (ground, vegetation, etc.) that you need\n"
			//	"3. Select a training set for each of these labels\n"
			//	"4. Train the classifier\n"
			//	"\n"
			//	"You can then either select more inliers for training and train again\n"
			//	"to improve the results, classify with or without regularization or\n"
			//	"save the classifier's configuration.\n"
			//	"\n"
			//	"When loading a classifier's configuration, the computed features\n"
			//	"should be the same (same number of scales, etc.) and the labels should\n"
			//	"be the same as when the classifier's configuration was saved.\n"
			//	"\n"
			//	"[IMPORTANT NOTICE ON SAVING CLASSIFIED ITEMS]\n"
			//	"\n"
			//	"If you intend to save the file after classifying, PLEASE CLOSE THE\n"
			//	"CLASSIFICATION PLUGIN FIRST: for visualization, colors are saved in\n"
			//	"the point set. If you do not close the classification plugin, colors\n"
			//	"will be saved and might overwrite existing colors of the point cloud.\n"
			//	"\n"
			//	"Classification results will be saved if you use the PLY\n"
			//	"formats. Training will be saved if you use the PLY format.\n"));
			QString("3D Annotator\n"
				"\n"
				"Welcome to 3D Annotator! Please read carefully this notice\n"
				"before using the plugin.\n"
				"\n"
				"[QUICK INTRODUCTION]\n"
				"You need to click the 'selection' layer in 'Geometric Objects' to start annotation process. After you finished, do not forget to save your work.\n"
				"\n"
				"In order to annotate the data, you might need to use the following shortcuts:\n"
				"[Most used]\n"
				"Shift + Left Button: Selection\n"
				"Shift + Wheel(Forward/Backward): Expand/Reduce selected area\n"
				"Shift + D + Left Button: Deselection\n"
				"Shift + [Capital letter of labels in the bracket(?)] : Add selection to the label category\n"
				"Ctrl + 1: Switch on/off Texture rendering mode\n"
				"Ctrl + 2: Switch on/off Flat rendering mode\n"
				"Ctrl + Left Button: Update to the new view center\n"
				"Ctrl + S: Save data\n"
				"Z + Mouse Left Button: Zoom into the position\n"
				"[Additional used]\n"
				"Left Button Double Click: Set orthographic projection\n"
				"Shift + Right Button: Pop up context menu of current layer\n"
				"Ctrl + Space: Turn on/off current view\n"
				"Ctrl + R: Recenter the view center to original\n"
				"Ctrl + L: Load data\n"
				"Ctrl + F1: Save snapshot and camera parameters\n"
				"Ctrl + F2: Load camera parameter and move to the view\n"
				"D + Mouse Left Button 1st && 2nd: Measure euclidean distance between 1st and 2nd point\n"
			));
	}

	void on_compute_features_button_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QMultipleInputDialog dialog("Compute Features", mw);
		QSpinBox* scales = dialog.add<QSpinBox>("Number of scales:");
		scales->setRange(1, 99);
		scales->setValue(5);

		QDoubleSpinBox* voxel_size = dialog.add<QDoubleSpinBox>("Voxel size (0 for automatic):");
		voxel_size->setRange(0.0, 10000.0);
		voxel_size->setValue(0.0);
		voxel_size->setSingleStep(0.01);

		if (dialog.exec() != QDialog::Accepted)
			return;

		QApplication::setOverrideCursor(Qt::WaitCursor);

		float vsize = float(voxel_size->value());
		if (vsize == 0.f)
			vsize = -1.f; // auto value

		classif->compute_features(std::size_t(scales->value()), vsize);

		update_plugin_from_item(classif);
		QApplication::restoreOverrideCursor();
		item_changed(classif->item());
	}

	void on_save_config_button_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QString filename;

		int classifier = get_classifier();
		if (classifier == 0) // Sum of Weighted Featuers
			filename = QFileDialog::getSaveFileName(mw,
				tr("Save classification configuration"),
				tr("%1 (classif config).xml").arg(classif->item()->name()),
				"classification configuration (*.xml);;");
		else if (classifier == 1) // Random Forest (ETHZ)
			filename = QFileDialog::getSaveFileName(mw,
				tr("Save classification configuration"),
				tr("%1 (ETHZ random forest config).gz").arg(classif->item()->name()),
				"Compressed ETHZ random forest configuration (*.gz);;");
#ifdef CGAL_LINKED_WITH_OPENCV
		else if (classifier == 2) // Random Forest (OpenCV)
			filename = QFileDialog::getSaveFileName(mw,
				tr("Save classification configuration"),
				tr("%1 (OpenCV %2.%3 random forest config).xml")
				.arg(classif->item()->name())
				.arg(CV_MAJOR_VERSION)
				.arg(CV_MINOR_VERSION),
				tr("OpenCV %2.%3 random forest configuration (*.xml);;")
				.arg(CV_MAJOR_VERSION)
				.arg(CV_MINOR_VERSION));
#endif
#ifdef CGAL_LINKED_WITH_TENSORFLOW
		else if (classifier == 3) // Neural Network (TensorFlow)
			filename = QFileDialog::getSaveFileName(mw,
				tr("Save classification configuration"),
				tr("%1 (Neural Network config).xml").arg(classif->item()->name()),
				"TensorFlow Neural Network classification configuration (*.xml);;");
#endif

		if (filename == QString())
			return;


		QApplication::setOverrideCursor(Qt::WaitCursor);

		classif->save_config(filename.toStdString().c_str(), classifier);

		QApplication::restoreOverrideCursor();

	}

	void on_load_config_button_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}
		QString filename;

		int classifier = get_classifier();
		if (classifier == 0) // SOWF
			filename = QFileDialog::getOpenFileName(mw,
				tr("Open classification configuration"),
				".",
				"classification configuration (*.xml);;All Files (*)");
		else if (classifier == 1) // ETHZ
			filename = QFileDialog::getOpenFileName(mw,
				tr("Open ETHZ random forest configuration"),
				".",
				"Compressed ETHZ random forest configuration (*.gz);;All Files (*)");
#ifdef CGAL_LINKED_WITH_OPENCV
		else if (classifier == 2) // OpenCV
			filename = QFileDialog::getOpenFileName(mw,
				tr("Open OpenCV %2.%3 random forest configuration")
				.arg(CV_MAJOR_VERSION)
				.arg(CV_MINOR_VERSION),
				".",
				tr("OpenCV %2.%3 random forest configuration (*.xml);;All Files (*)")
				.arg(CV_MAJOR_VERSION)
				.arg(CV_MINOR_VERSION));
#endif
#ifdef CGAL_LINKED_WITH_TENSORFLOW
		else if (classifier == 3) // TensorFlow
			filename = QFileDialog::getOpenFileName(mw,
				tr("Open Neural Network classification configuration"),
				".",
				tr("Neural Network classification configuration (*.xml);;All Files (*)"));
#endif

		if (filename == QString())
			return;

		QApplication::setOverrideCursor(Qt::WaitCursor);

		classif->load_config(filename.toStdString().c_str(), classifier);

		update_plugin_from_item(classif);
		run(classif, 0);

		QApplication::restoreOverrideCursor();
		item_changed(classif->item());
	}

	// the old version of change_color
	//void change_color(Item_classification_base* classif, int index)
	//{
	//	float vmin = std::numeric_limits<float>::infinity();
	//	float vmax = std::numeric_limits<float>::infinity();
	//	classif->change_color(index, &vmin, &vmax);
	//	if (vmin == std::numeric_limits<float>::infinity() || vmax == std::numeric_limits<float>::infinity())
	//	{
	//		ui_widget.minDisplay->setEnabled(false);
	//		ui_widget.minDisplay->setText("Min");
	//		ui_widget.maxDisplay->setEnabled(false);
	//		ui_widget.maxDisplay->setText("Max");
	//	}
	//	else
	//	{
	//		ui_widget.minDisplay->setEnabled(true);
	//		ui_widget.minDisplay->setText(tr("Min (%1)").arg(vmin));
	//		ui_widget.maxDisplay->setEnabled(true);
	//		ui_widget.maxDisplay->setText(tr("Max (%1)").arg(vmax));
	//	}

	//	item_changed(classif->item());
	//}


	//void on_display_button_clicked(int index)
	//{
	//	Item_classification_base* classif
	//		= get_classification();
	//	if (!classif)
	//		return;
	//	print_message("here in the on_dispaly func.");
	//	change_color(classif, index);
	//}

	float display_button_value(QPushButton* button)
	{
		std::string text = button->text().toStdString();

		std::size_t pos1 = text.find('(');
		if (pos1 == std::string::npos)
			return std::numeric_limits<float>::infinity();
		std::size_t pos2 = text.find(')');
		if (pos2 == std::string::npos)
			return std::numeric_limits<float>::infinity();

		std::string fstring(text.begin() + pos1 + 1,
			text.begin() + pos2);

		return float(std::atof(fstring.c_str()));
	}

	//void on_min_display_button_clicked()
	//{
	//	Item_classification_base* classif
	//		= get_classification();
	//	if (!classif)
	//		return;

	//	float vmin = display_button_value(ui_widget.minDisplay);
	//	float vmax = display_button_value(ui_widget.maxDisplay);

	//	if (vmin == std::numeric_limits<float>::infinity()
	//		|| vmax == std::numeric_limits<float>::infinity())
	//		return;

	//	bool ok = false;
	//	vmin = float(QInputDialog::getDouble((QWidget*)mw,
	//		tr("Set display ramp minimum value (saturate under):"),
	//		tr("Minimum value (pale blue):"),
	//		double(vmin),
	//		-10000000.0,
	//		double(vmax), 5, &ok));
	//	if (!ok)
	//		return;

	//	int index = ui_widget.display->currentIndex();

	//	classif->change_color(index, &vmin, &vmax);
	//	ui_widget.minDisplay->setText(tr("Min* (%1)").arg(vmin));

	//	item_changed(classif->item());
	//}

	//void on_max_display_button_clicked()
	//{
	//	Item_classification_base* classif
	//		= get_classification();
	//	if (!classif)
	//		return;

	//	float vmin = display_button_value(ui_widget.minDisplay);
	//	float vmax = display_button_value(ui_widget.maxDisplay);

	//	if (vmin == std::numeric_limits<float>::infinity()
	//		|| vmax == std::numeric_limits<float>::infinity())
	//		return;

	//	bool ok = false;
	//	vmax = float(QInputDialog::getDouble((QWidget*)mw,
	//		tr("Set display ramp maximum value (saturate over):"),
	//		tr("Maximum value (dark red):"),
	//		double(vmax),
	//		double(vmin),
	//		10000000.0, 5, &ok));
	//	if (!ok)
	//		return;

	//	int index = ui_widget.display->currentIndex();

	//	classif->change_color(index, &vmin, &vmax);
	//	ui_widget.maxDisplay->setText(tr("Max* (%1)").arg(vmax));

	//	item_changed(classif->item());
	//}

	void on_run_button_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}
		QApplication::setOverrideCursor(Qt::WaitCursor);
		CGAL::Real_timer t;
		t.start();
		run(classif, 0);
		t.stop();
		std::cerr << "Raw classification computed in " << t.time() << " second(s)" << std::endl;
		QApplication::restoreOverrideCursor();
		item_changed(classif->item());
	}

	void on_run_smoothed_button_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QApplication::setOverrideCursor(Qt::WaitCursor);
		CGAL::Real_timer t;
		t.start();
		run(classif, 1);
		t.stop();
		std::cerr << "Smoothed classification computed in " << t.time() << " second(s)" << std::endl;
		QApplication::restoreOverrideCursor();
		item_changed(classif->item());
	}

	void on_run_graphcut_button_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QMultipleInputDialog dialog("Classify with Graph Cut", mw);
		QSpinBox* subdivisions = dialog.add<QSpinBox>("Number of subdivisons: ");
		subdivisions->setRange(1, 9999);
		subdivisions->setValue(16);

		QDoubleSpinBox* smoothing = dialog.add<QDoubleSpinBox>("Regularization weight: ");
		smoothing->setRange(0.0, 100.0);
		smoothing->setValue(0.5);
		smoothing->setSingleStep(0.1);

		if (dialog.exec() != QDialog::Accepted)
			return;

		QApplication::setOverrideCursor(Qt::WaitCursor);
		CGAL::Real_timer t;
		t.start();
		run(classif, 2, std::size_t(subdivisions->value()), smoothing->value());
		t.stop();
		std::cerr << "Graph Cut classification computed in " << t.time() << " second(s)" << std::endl;
		QApplication::restoreOverrideCursor();
		item_changed(classif->item());
	}

	void on_create_point_set_item()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QPushButton* label_clicked = qobject_cast<QPushButton*>(QObject::sender()->parent()->parent());

		if (label_clicked == NULL)
			std::cerr << "Error" << std::endl;
		else
		{
			int index = ui_widget.labelGrid->indexOf(label_clicked);
			int row_index, column_index, row_span, column_span;
			ui_widget.labelGrid->getItemPosition(index, &row_index, &column_index, &row_span, &column_span);

			int position = row_index * 3 + column_index;

			Scene_item* item = classif->generate_one_item
			(classif->item()->name().toStdString().c_str(), position);

			Scene_points_with_normal_item* points_item
				= qobject_cast<Scene_points_with_normal_item*>(item);

			if (!points_item)
				return;

			if (points_item->point_set()->empty())
				delete points_item;
			else
				scene->addItem(points_item);
		}
	}

	void on_generate_items_button_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}


		QApplication::setOverrideCursor(Qt::WaitCursor);

		std::vector<Scene_item*> new_items;
		classif->generate_one_item_per_label
		(new_items, classif->item()->name().toStdString().c_str());

		for (std::size_t i = 0; i < new_items.size(); ++i)
		{
			Scene_points_with_normal_item* points_item
				= qobject_cast<Scene_points_with_normal_item*>(new_items[i]);
			if (!points_item)
				continue;

			if (points_item->point_set()->empty())
				delete points_item;
			else
				scene->addItem(points_item);
		}

		QApplication::restoreOverrideCursor();
	}

	void clear_labels(Item_classification_base* classif)
	{
		classif->clear_labels();
	}

	void add_new_label(Item_classification_base* classif, const std::string& name)
	{
		add_new_label(LabelButton(dock_widget,
			name.c_str(),
			QColor(64 + rand() % 192,
				64 + rand() % 192,
				64 + rand() % 192),
			get_shortcut(label_buttons.size(), name.c_str())));
		QColor color = classif->add_new_label(name.c_str());
		label_buttons.back().change_color(color);
	}

	void on_add_new_label_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		bool ok;
		QString name =
			QInputDialog::getText((QWidget*)mw,
				tr("Add new labels"), // dialog title
				tr("Names (separated by spaces):"), // field label
				QLineEdit::Normal,
				tr("label_%1").arg(label_buttons.size()),
				&ok);
		if (!ok)
			return;

		std::istringstream iss(name.toStdString());

		std::string n;
		while (iss >> n)
			add_new_label(classif, n);

		add_label_button();
		update_plugin_from_item(classif);
	}

	void on_use_config_building_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		if (classif->number_of_labels() != 0)
		{
			QMessageBox::StandardButton reply
				= QMessageBox::question(NULL, "Classification",
					"Current labels will be discarded. Continue?",
					QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

			if (reply == QMessageBox::No)
				return;
		}
		clear_labels(classif);
		add_new_label(classif, "ground");
		add_new_label(classif, "vegetation");
		add_new_label(classif, "building");
		update_plugin_from_item(classif);
	}

	void on_use_config_roof_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		if (classif->number_of_labels() != 0)
		{
			QMessageBox::StandardButton reply
				= QMessageBox::question(NULL, "Classification",
					"Current labels will be discarded. Continue?",
					QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

			if (reply == QMessageBox::No)
				return;
		}
		clear_labels(classif);
		add_new_label(classif, "ground");
		add_new_label(classif, "vegetation");
		add_new_label(classif, "roof");
		add_new_label(classif, "facade");
		update_plugin_from_item(classif);
	}

	void on_use_las_config_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		if (classif->number_of_labels() != 0)
		{
			QMessageBox::StandardButton reply
				= QMessageBox::question(NULL, "Classification",
					"Current labels will be discarded. Continue?",
					QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

			if (reply == QMessageBox::No)
				return;
		}
		clear_labels(classif);
		add_new_label(classif, "ground");
		add_new_label(classif, "low_veget");
		add_new_label(classif, "med_veget");
		add_new_label(classif, "high_veget");
		add_new_label(classif, "building");
		add_new_label(classif, "noise");
		add_new_label(classif, "reserved");
		add_new_label(classif, "water");
		add_new_label(classif, "rail");
		add_new_label(classif, "road_surface");
		add_new_label(classif, "reserved_2");
		add_new_label(classif, "wire_guard");
		add_new_label(classif, "wire_conduct");
		add_new_label(classif, "trans_tower");
		add_new_label(classif, "wire_connect");
		add_new_label(classif, "bridge_deck");
		add_new_label(classif, "high_noise");
		update_plugin_from_item(classif);
	}

	void on_clear_labels_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		if (classif->number_of_labels() != 0)
		{
			QMessageBox::StandardButton reply
				= QMessageBox::question(NULL, "Classification",
					"Current labels will be discarded. Continue?",
					QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

			if (reply == QMessageBox::No)
				return;
		}
		clear_labels(classif);
		update_plugin_from_item(classif);
	}

	void on_reset_training_sets_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		classif->reset_training_sets();

		item_changed(classif->item());
	}

	void on_reset_training_set_of_selection_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		classif->reset_training_set_of_selection();

		item_changed(classif->item());
	}

	void on_reset_training_set_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QPushButton* label_clicked = qobject_cast<QPushButton*>(QObject::sender()->parent()->parent());
		if (label_clicked == NULL)
			std::cerr << "Error" << std::endl;
		else
		{
			int index = ui_widget.labelGrid->indexOf(label_clicked);
			int row_index, column_index, row_span, column_span;
			ui_widget.labelGrid->getItemPosition(index, &row_index, &column_index, &row_span, &column_span);

			int position = row_index * 3 + column_index;

			classif->reset_training_set
			(position);
		}

		item_changed(classif->item());
	}

	void on_validate_selection_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		classif->validate_selection();
	}

	void on_select_random_region_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		classif->select_random_region();
		item_changed(classif->item());

		CGAL::QGLViewer* viewer = *CGAL::QGLViewer::QGLViewerPool().begin();
		CGAL::Bbox_3 bbox = classif->bbox();
		const CGAL::qglviewer::Vec offset = static_cast<CGAL::Three::Viewer_interface*>(viewer)->offset();

		viewer->camera()->fitBoundingBox(CGAL::qglviewer::Vec(bbox.xmin(), bbox.ymin(), bbox.zmin()) + offset,
			CGAL::qglviewer::Vec(bbox.xmax(), bbox.ymax(), bbox.zmax()) + offset);



		viewer->camera()->setPivotPoint(CGAL::qglviewer::Vec((bbox.xmin() + bbox.xmax()) / 2.,
			(bbox.ymin() + bbox.ymax()) / 2.,
			(bbox.zmin() + bbox.zmax()) / 2.) + offset);
	}

	void on_statistics_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QApplication::setOverrideCursor(Qt::WaitCursor);
		std::string str = classif->feature_statistics();
		QApplication::restoreOverrideCursor();

		QMultipleInputDialog dialog("Feature Statistics", mw);
		QLabel* text = dialog.add<QLabel>("");
		text->setText(str.c_str());
		dialog.exec_no_cancel();
	}

	void on_switch_classifier_clicked()
	{
		QMultipleInputDialog dialog("Which classifier do you want to use?", mw);

		QRadioButton* ethz = dialog.add<QRadioButton>(CGAL_CLASSIFICATION_ETHZ_ID);
		ethz->setChecked(true);

		QRadioButton* sowf = dialog.add<QRadioButton>(CGAL_CLASSIFICATION_SOWF_ID);

#ifdef CGAL_LINKED_WITH_TENSORFLOW
		QRadioButton* tensorflow = dialog.add<QRadioButton>(CGAL_CLASSIFICATION_TENSORFLOW_ID);
#endif

#ifdef CGAL_LINKED_WITH_OPENCV
		QRadioButton* opencv = dialog.add<QRadioButton>(CGAL_CLASSIFICATION_OPENCV_ID);
#endif

		if (dialog.exec() != QDialog::Accepted)
			return;

		if (ethz->isChecked())
			classifier->setText(CGAL_CLASSIFICATION_ETHZ_ID);
		else if (sowf->isChecked())
			classifier->setText(CGAL_CLASSIFICATION_SOWF_ID);
#ifdef CGAL_LINKED_WITH_TENSORFLOW
		else if (tensorflow->isChecked())
			classifier->setText(CGAL_CLASSIFICATION_TENSORFLOW_ID);
#endif
#ifdef CGAL_LINKED_WITH_OPENCV
		else if (opencv->isChecked())
			classifier->setText(CGAL_CLASSIFICATION_OPENCV_ID);
#endif

		if (sowf->isChecked())
		{
			dock_widget_adv->show();
			dock_widget_adv->raise();
		}
		else
			dock_widget_adv->hide();
	}

	void on_train_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QMultipleInputDialog dialog("Train Classifier", mw);

		int classifier = get_classifier();
		if (classifier == 0) // SOWF
		{
			QSpinBox* trials = dialog.add<QSpinBox>("Number of trials: ", "trials");
			trials->setRange(1, 99999);
			trials->setValue(800);
		}
		else if (classifier == 1 || classifier == 2) // random forest
		{
			QSpinBox* trees = dialog.add<QSpinBox>("Number of trees: ", "num_trees");
			trees->setRange(1, 9999);
			trees->setValue(25);
			QSpinBox* depth = dialog.add<QSpinBox>("Maximum depth of tree: ", "max_depth");
			depth->setRange(1, 9999);
			depth->setValue(20);
		}
		else if (classifier == 3) // neural network
		{
			QSpinBox* trials = dialog.add<QSpinBox>("Number of trials: ", "trials");
			trials->setRange(1, 99999);
			trials->setValue(500);
			QDoubleSpinBox* rate = dialog.add<QDoubleSpinBox>("Learning rate: ", "learning_rate");
			rate->setRange(0.00001, 10000.0);
			rate->setValue(0.001);
			rate->setDecimals(5);
			QSpinBox* batch = dialog.add<QSpinBox>("Batch size: ", "batch_size");
			batch->setRange(1, 2000000000);
			batch->setValue(1000);
			dialog.add<QLineEdit>("Hidden layer size(s): ", "hidden_layers");
			QCheckBox* restart = dialog.add<QCheckBox>("Restart from scratch: ", "restart");
			restart->setChecked(false);
		}

		if (dialog.exec() != QDialog::Accepted)
			return;

		QApplication::setOverrideCursor(Qt::WaitCursor);
		CGAL::Real_timer t;
		t.start();
		classif->train(classifier, dialog);
		t.stop();
		std::cerr << "Done in " << t.time() << " second(s)" << std::endl;
		QApplication::restoreOverrideCursor();
		update_plugin_from_item(classif);
	}

	char get_shortcut(std::size_t position, const char* name)
	{
		std::set<char> used_letters;

		used_letters.insert('t'); // used for "train"
		used_letters.insert('s'); // used for "random select"
		for (std::size_t i = 0; i < label_buttons.size(); ++i)
			if (i != position)
				used_letters.insert(label_buttons[i].shortcut);

		std::size_t idx = 0;
		while (name[idx] != '\0')
		{
			if (std::isalpha(name[idx]) &&
				used_letters.find(std::tolower(name[idx])) == used_letters.end())
				return std::tolower(name[idx]);
			++idx;
		}

		char fallback = 'a';
		while (used_letters.find(fallback) != used_letters.end())
			++fallback;

		return fallback;
	}

	void add_new_label(const LabelButton& label_button)
	{
		label_buttons.push_back(label_button);
		int position = static_cast<int>(label_buttons.size()) - 1;

		int x = position / 3;
		int y = position % 3;

		ui_widget.labelGrid->addWidget(label_buttons.back().color_button, x, y);

		QAction* add_selection = label_buttons.back().menu->addAction("Add selection to training set");

		add_selection->setShortcut(Qt::SHIFT | (Qt::Key_A + (label_button.shortcut - 'a')));
		//    add_selection->setShortcut(Qt::Key_0 + label_buttons.size() - 1);

		connect(add_selection, SIGNAL(triggered()), this,
			SLOT(on_add_selection_to_training_set_clicked()));

		QAction* reset = label_buttons.back().menu->addAction("Reset training set");
		//***********************Weixiao Update menu name*********************//
		reset->setDisabled(true);
		reset->setVisible(false);
		//********************************************************************//
		connect(reset, SIGNAL(triggered()), this,
			SLOT(on_reset_training_set_clicked()));

		label_buttons.back().menu->addSeparator();

		QAction* change_color = label_buttons.back().menu->addAction("Change color");
		//***********************Weixiao Update menu name*********************//
		//change_color->setDisabled(true);
		//change_color->setVisible(false);
		//********************************************************************//
		connect(change_color, SIGNAL(triggered()), this,
			SLOT(on_color_changed_clicked()));

		QAction* change_name = label_buttons.back().menu->addAction("Change name");
		//***********************Weixiao Update menu name*********************//
		change_name->setDisabled(true);
		change_name->setVisible(false);
		//********************************************************************//
		connect(change_name, SIGNAL(triggered()), this,
			SLOT(on_name_changed_clicked()));

		QAction* create = label_buttons.back().menu->addAction("Create point set item from labeled points");
		//***********************Weixiao Update menu name*********************//
		create->setDisabled(true);
		create->setVisible(false);
		//********************************************************************//
		connect(create, SIGNAL(triggered()), this,
			SLOT(on_create_point_set_item()));

		label_buttons.back().menu->addSeparator();

		QAction* remove_label = label_buttons.back().menu->addAction("Remove label");
		//***********************Weixiao Update menu name*********************//
		remove_label->setDisabled(true);
		remove_label->setVisible(false);
		//********************************************************************//
		connect(remove_label, SIGNAL(triggered()), this,
			SLOT(on_remove_label_clicked()));

		ui_widget_adv.gridLayout->addWidget(label_buttons.back().label2, position + 1, 0);
		ui_widget_adv.gridLayout->addWidget(label_buttons.back().effect, position + 1, 2);

		connect(label_buttons.back().effect, SIGNAL(currentIndexChanged(int)), this,
			SLOT(on_effect_changed(int)));
	}

	void add_label_button()
	{
		int position = static_cast<int>(label_buttons.size());
		int x = position / 3;
		int y = position % 3;

		label_button->setVisible(true);
		ui_widget.labelGrid->addWidget(label_button, x, y);
	}

	void on_remove_label_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QPushButton* label_clicked = qobject_cast<QPushButton*>(QObject::sender()->parent()->parent());
		if (label_clicked == NULL)
			std::cerr << "Error" << std::endl;
		else
		{
			int index = ui_widget.labelGrid->indexOf(label_clicked);
			int row_index, column_index, row_span, column_span;
			ui_widget.labelGrid->getItemPosition(index, &row_index, &column_index, &row_span, &column_span);

			int position = row_index * 3 + column_index;

			classif->remove_label(position);

			ui_widget.labelGrid->removeWidget(label_buttons[position].color_button);
			label_buttons[position].color_button->deleteLater();
			label_buttons[position].menu->deleteLater();

			ui_widget_adv.gridLayout->removeWidget(label_buttons[position].label2);
			delete label_buttons[position].label2;
			ui_widget_adv.gridLayout->removeWidget(label_buttons[position].effect);
			delete label_buttons[position].effect;

			if (label_buttons.size() > 1)
				for (int i = position + 1; i < static_cast<int>(label_buttons.size()); ++i)
				{
					int position = i - 1;
					int x = position / 3;
					int y = position % 3;

					ui_widget.labelGrid->addWidget(label_buttons[i].color_button, x, y);
					ui_widget_adv.gridLayout->addWidget(label_buttons[i].label2, (int)i, 0);
					ui_widget_adv.gridLayout->addWidget(label_buttons[i].effect, (int)i, 2);
				}

			label_buttons.erase(label_buttons.begin() + position);
			add_label_button();
		}
		update_plugin_from_item(classif);
		item_changed(classif->item());
	}

	void on_color_changed_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QPushButton* label_clicked = qobject_cast<QPushButton*>(QObject::sender()->parent()->parent());
		if (label_clicked == NULL)
			std::cerr << "Error" << std::endl;
		else
		{
			int index = ui_widget.labelGrid->indexOf(label_clicked);
			int row_index, column_index, row_span, column_span;
			ui_widget.labelGrid->getItemPosition(index, &row_index, &column_index, &row_span, &column_span);

			int position = row_index * 3 + column_index;

			QColor color = label_buttons[position].color;
			color = QColorDialog::getColor(color, (QWidget*)mw, "Change color of label");

			if (!color.isValid())
				return;

			label_buttons[position].change_color(color);
			classif->change_label_color(position,
				color);
		}
		classif->update_color();
		item_changed(classif->item());
	}

	void on_name_changed_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QPushButton* label_clicked = qobject_cast<QPushButton*>(QObject::sender()->parent()->parent());
		if (label_clicked == NULL)
			std::cerr << "Error" << std::endl;
		else
		{
			int index = ui_widget.labelGrid->indexOf(label_clicked);
			int row_index, column_index, row_span, column_span;
			ui_widget.labelGrid->getItemPosition(index, &row_index, &column_index, &row_span, &column_span);

			int position = row_index * 3 + column_index;

			bool ok;
			QString name =
				QInputDialog::getText((QWidget*)mw,
					tr("Change name of label"), // dialog title
					tr("New name:"), // field label
					QLineEdit::Normal,
					classif->label(position)->name().c_str(),
					&ok);

			if (!ok)
				return;

			classif->change_label_name(position, name.toStdString());
		}

		update_plugin_from_item(classif);
		classif->update_color();
		item_changed(classif->item());
	}

	void on_add_selection_to_training_set_clicked()
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}

		QPushButton* label_clicked = qobject_cast<QPushButton*>(QObject::sender()->parent()->parent());
		if (label_clicked == NULL)
			std::cerr << "Error" << std::endl;
		else
		{
			int index = ui_widget.labelGrid->indexOf(label_clicked);
			int row_index, column_index, row_span, column_span;
			ui_widget.labelGrid->getItemPosition(index, &row_index, &column_index, &row_span, &column_span);

			int position = row_index * 3 + column_index;
			/*****************************Ziqian*******************************/
			if (!classif->segment_form()) {
				print_message("Error: can't put faces from different segment into one segment.");
				//return;
			}
			/******************************************************************/

			classif->add_selection_to_training_set(position);
			//*****************************Weixiao**********************//
			int total_facet_num = classif->get_total_number_facets();
			ui_widget.lineEdit->setText(QString::number(total_facet_num));
			int unlabelled_num = classif->get_unlabelled_number_facets();
			ui_widget.lineEdit_2->setText(QString::number(unlabelled_num));
			float labeling_progress = 1.0f - float(unlabelled_num) / float(total_facet_num);
			labeling_progress = total_facet_num == unlabelled_num ? 0.0f : labeling_progress * 100.0f;
			ui_widget.progressBar->setValue(labeling_progress);

			Scene_polyhedron_selection_item* selection_item
				= qobject_cast<Scene_polyhedron_selection_item*>(scene->item(scene->mainSelectionIndex()));
			if (selection_item)
				selection_item->polyhedron_item()->update_labels_for_selection();
			//**********************************************************//
			//**********************************************************//
		}
		item_changed(classif->item());
	}

	void on_selected_feature_changed(int v)
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
			return;

		if (classif->number_of_features() <= (std::size_t)v)
			return;

		Item_classification_base::Feature_handle
			att = classif->feature(v);

		if (att == Item_classification_base::Feature_handle())
			return;

		ui_widget_adv.feature_weight->setValue((int)(1001. * 2. * std::atan(classif->weight(att)) / CGAL_PI));

		for (std::size_t i = 0; i < classif->number_of_labels(); ++i)
		{
			CGAL::Classification::Sum_of_weighted_features_classifier::Effect
				eff = classif->effect(classif->label(i), att);
			if (eff == CGAL::Classification::Sum_of_weighted_features_classifier::PENALIZING)
				label_buttons[i].effect->setCurrentIndex(0);
			else if (eff == CGAL::Classification::Sum_of_weighted_features_classifier::NEUTRAL)
				label_buttons[i].effect->setCurrentIndex(1);
			else
				label_buttons[i].effect->setCurrentIndex(2);
		}
	}

	void on_feature_weight_changed(int v)
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}
		Item_classification_base::Feature_handle
			att = classif->feature(ui_widget_adv.selected_feature->currentIndex());

		if (att == Item_classification_base::Feature_handle())
			return;

		classif->set_weight(att, std::tan((CGAL_PI / 2.) * v / 1001.));

		for (std::size_t i = 0; i < label_buttons.size(); ++i)
			label_buttons[i].effect->setEnabled(classif->weight(att) != 0.);
	}

	void on_effect_changed(int v)
	{
		Item_classification_base* classif
			= get_classification();
		if (!classif)
		{
			print_message("Error: there is no point set classification item!");
			return;
		}
		Item_classification_base::Feature_handle
			att = classif->feature(ui_widget_adv.selected_feature->currentIndex());

		if (att == Item_classification_base::Feature_handle())
			return;

		QComboBox* combo = qobject_cast<QComboBox*>(QObject::sender());
		for (std::size_t i = 0; i < label_buttons.size(); ++i)
			if (label_buttons[i].effect == combo)
			{
				//          std::cerr << att->id() << " is ";
				if (v == 0)
				{
					classif->set_effect(classif->label(i),
						att, CGAL::Classification::Sum_of_weighted_features_classifier::PENALIZING);
				}
				else if (v == 1)
				{
					classif->set_effect(classif->label(i),
						att, CGAL::Classification::Sum_of_weighted_features_classifier::NEUTRAL);
				}
				else
				{
					classif->set_effect(classif->label(i),
						att, CGAL::Classification::Sum_of_weighted_features_classifier::FAVORING);
				}
				break;
			}
	}

	//***********************Ziqian*******************************//
	bool threshold_based_change_color(Item_classification_base* classif, int index, int threshold, bool below)
	{
		float vmin = std::numeric_limits<float>::infinity();
		float vmax = std::numeric_limits<float>::infinity();

		classif->threshold_based_change_color(index, threshold, below, &vmin, &vmax);

		//if (vmin == std::numeric_limits<float>::infinity() || vmax == std::numeric_limits<float>::infinity())
		//{
		//	ui_widget.minDisplay->setEnabled(false);
		//	ui_widget.minDisplay->setText("Min");
		//	ui_widget.maxDisplay->setEnabled(false);
		//	ui_widget.maxDisplay->setText("Max");
		//}
		//else
		//{
		//	ui_widget.minDisplay->setEnabled(true);
		//	ui_widget.minDisplay->setText(tr("Min (%1)").arg(vmin));
		//	ui_widget.maxDisplay->setEnabled(true);
		//	ui_widget.maxDisplay->setText(tr("Max (%1)").arg(vmax));
		//}

		item_changed(classif->item());
		return true;
	}

	void disable_probability_enable_progressbar()
	{
		//probability slider
		ui_widget.ProbSlider->setEnabled(false);
		ui_widget.ProbSpin->setEnabled(false);
		ui_widget.ProbSwitcher->setEnabled(false);
		ui_widget.label_2->setEnabled(false);
		ui_widget.label->setEnabled(false);
		//progress bar
		ui_widget.label_3->setEnabled(true);
		ui_widget.label_4->setEnabled(true);
		ui_widget.label_5->setEnabled(true);
		ui_widget.lineEdit->setEnabled(true);
		ui_widget.lineEdit_2->setEnabled(true);
		ui_widget.progressBar->setEnabled(true);
	}

	void enable_probability_disable_progressbar()
	{
		//probability slider
		ui_widget.ProbSlider->setEnabled(true);
		ui_widget.ProbSpin->setEnabled(true);
		ui_widget.ProbSwitcher->setEnabled(true);
		ui_widget.label_2->setEnabled(true);
		ui_widget.label->setEnabled(true);
		//progress bar
		ui_widget.label_3->setEnabled(false);
		ui_widget.label_4->setEnabled(false);
		ui_widget.label_5->setEnabled(false);
		ui_widget.lineEdit->setEnabled(false);
		ui_widget.lineEdit_2->setEnabled(false);
		ui_widget.progressBar->setEnabled(false);
	}

	void disable_probability_disable_progressbar()
	{
		//probability slider
		ui_widget.ProbSlider->setEnabled(false);
		ui_widget.ProbSpin->setEnabled(false);
		ui_widget.ProbSwitcher->setEnabled(false);
		ui_widget.label_2->setEnabled(false);
		ui_widget.label->setEnabled(false);
		//progress bar
		ui_widget.label_3->setEnabled(false);
		ui_widget.label_4->setEnabled(false);
		ui_widget.label_5->setEnabled(false);
		ui_widget.lineEdit->setEnabled(false);
		ui_widget.lineEdit_2->setEnabled(false);
		ui_widget.progressBar->setEnabled(false);
	}

	void on_display_button_clicked_with_probability(int index)
	{
		Item_classification_base* classif = get_classification();
		if (!classif) return;

		if (index == 0)
			disable_probability_disable_progressbar();
		else
		{
			if (!classif->can_show_probability())
				disable_probability_enable_progressbar();
			else
				enable_probability_disable_progressbar();
		}

		int threshold = ui_widget.ProbSlider->value();
		bool below;
		if (ui_widget.ProbSwitcher->currentIndex() == 0)
			below = true;
		else
			below = false;

		threshold_based_change_color(classif, index, threshold, below);
	}


	void on_probability_threshold_changed(int value) {
		Item_classification_base* classif = get_classification();
		if (!classif) return;
		int threshold = value;
		bool below;
		if (ui_widget.ProbSwitcher->currentIndex() == 0) below = true;
		else below = false;

		int index = ui_widget.display->currentIndex();

		threshold_based_change_color(classif, index, threshold, below);
		//print_message("probability_state_changed!");
	}

	void on_probability_switcher_changed(int value) {
		Item_classification_base* classif = get_classification();
		if (!classif) return;
		int threshold = ui_widget.ProbSlider->value();
		bool below;
		if (value == 0) below = true;
		else below = false;

		int index = ui_widget.display->currentIndex();

		threshold_based_change_color(classif, index, threshold, below);
		//print_message("probability_state_changed!");
	}
	//********************************************************************//
private:
	Messages_interface* messages;
	QAction* actionClassification;

	QDockWidget* dock_widget;
	QDockWidget* dock_widget_adv;
	QAction* action_statistics;
	QAction* action_reset_local;
	QAction* action_reset;
	QAction* action_random_region;
	QAction* action_validate;

	QAction* classifier;
	QAction* action_train;
	QAction* action_run;
	QAction* action_run_smoothed;
	QAction* action_run_graphcut;
	QAction* action_save_config;
	QAction* action_load_config;

	std::vector<LabelButton> label_buttons;
	QPushButton* label_button;

	Ui::Classification ui_widget;
	Ui::ClassificationAdvanced ui_widget_adv;

	QColor color_att;

	typedef std::map<Scene_item*, Item_classification_base*> Item_map;
	Item_map item_map;


}; // end Polyhedron_demo_classification_plugin

#include "Classification_plugin.moc"
