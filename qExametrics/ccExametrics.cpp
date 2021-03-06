//##########################################################################
//#                                                                        #
//#                       CLOUDCOMPARE PLUGIN: qExametrics                 #
//#                                                                        #
//##########################################################################

#include "ccExametrics.h"
#include "ccExametricsDialog.h"

//Qt
#include <QtGui>

/* Default constructor */
ccExametrics::ccExametrics(QObject* parent/*=0*/)
	: QObject(parent)
	, m_action(0)
{
}

/* Deconstructor */
ccExametrics::~ccExametrics()
{
	if (m_dlg)
		delete m_dlg;
}

//This method should enable or disable each plugin action
//depending on the currently selected entities ('selectedEntities').
//For example: if none of the selected entities is a cloud, and your
//plugin deals only with clouds, call 'm_action->setEnabled(false)'
void ccExametrics::onNewSelection(const ccHObject::Container& selectedEntities)
{
	//disable the main plugin icon if no entity is loaded
	m_action->setEnabled(m_app && m_app->dbRootObject() && m_app->dbRootObject()->getChildrenNumber() != 0);

	if (!m_dlg)
	{
		return; //not initialized yet - ignore callback
	}

	//m_app->dispToConsole("New Selection", ccMainAppInterface::STD_CONSOLE_MESSAGE);
}

//This method returns all 'actions' of your plugin.
//It will be called only once, when plugin is loaded.
void ccExametrics::getActions(QActionGroup& group)
{
	//default action (if it has not been already created, it's the moment to do it)
	if (!m_action)
	{
		//here we use the default plugin name, description and icon,
		//but each action can have its own!
		m_action = new QAction(getName(),this);
		m_action->setToolTip(getDescription());
		m_action->setIcon(getIcon());
		//connect appropriate signal
		connect(m_action, SIGNAL(triggered()), this, SLOT(doAction()));
	}

	group.addAction(m_action);

	m_app->dispToConsole("[ccExametrics] ccExametrics plugin initialized successfully.", ccMainAppInterface::STD_CONSOLE_MESSAGE);
}

//This is an example of an action's slot called when the corresponding action
//is triggered (i.e. the corresponding icon or menu entry is clicked in CC's
//main interface). You can access to most of CC components (database,
//3D views, console, etc.) via the 'm_app' attribute (ccMainAppInterface
//object).
void ccExametrics::doAction()
{
	//m_app should have already been initialized by CC when plugin is loaded!
	//(--> pure internal check)
	assert(m_app);
	if (!m_app)
		return;

	/*** HERE STARTS THE ACTION ***/


	//check valid window
	if (!m_app->getActiveGLWindow())
	{
		m_app->dispToConsole("[ccExametrics] Could not find valid 3D window.", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
		return;
	}

	//bind gui
	if (!m_dlg)
	{
		//bind GUI events
		m_dlg = new ccExametricsDialog((QWidget*)m_app->getMainWindow());

		//general
		ccExametricsDialog::connect(m_dlg->computeButton, SIGNAL(clicked()), this, SLOT(onCompute()));
		ccExametricsDialog::connect(m_dlg->closeButton, SIGNAL(clicked()), this, SLOT(onClose()));
		//parameter changed
		ccExametricsDialog::connect(m_dlg->spbXA, SIGNAL(valueChanged(double)), this, SLOT(onSpbXAChanged(double)));
		ccExametricsDialog::connect(m_dlg->spbYA, SIGNAL(valueChanged(double)), this, SLOT(onSpbYAChanged(double)));
		ccExametricsDialog::connect(m_dlg->spbZA, SIGNAL(valueChanged(double)), this, SLOT(onSpbZAChanged(double)));
		ccExametricsDialog::connect(m_dlg->spbXB, SIGNAL(valueChanged(double)), this, SLOT(onSpbXBChanged(double)));
		ccExametricsDialog::connect(m_dlg->spbYB, SIGNAL(valueChanged(double)), this, SLOT(onSpbYBChanged(double)));
		ccExametricsDialog::connect(m_dlg->spbZB, SIGNAL(valueChanged(double)), this, SLOT(onSpbZBChanged(double)));
		ccExametricsDialog::connect(m_dlg->spbX, SIGNAL(valueChanged(double)), this, SLOT(onSpbXChanged(double)));
		ccExametricsDialog::connect(m_dlg->spbY, SIGNAL(valueChanged(double)), this, SLOT(onSpbYChanged(double)));
		ccExametricsDialog::connect(m_dlg->spbZ, SIGNAL(valueChanged(double)), this, SLOT(onSpbZChanged(double)));
		ccExametricsDialog::connect(m_dlg->toleranceSpb, SIGNAL(valueChanged(double)), this, SLOT(onToleranceSpbChanged(double)));



		m_dlg->linkWith(m_app->getActiveGLWindow());

		// initialize spinbox with the object in db tree
		initializeSpinBox(m_app->dbRootObject()->getBB_recursive());

		// initialize polylines etc
		initializeDrawSettings();

		//start GUI
		m_app->registerOverlayDialog(m_dlg, Qt::TopRightCorner);
		m_dlg->start();
	}


	/*m_app->dispToConsole("[ccExametrics] Hello world!",ccMainAppInterface::STD_CONSOLE_MESSAGE); //a standard message is displayed in the console
	m_app->dispToConsole("[qExametricsPlugin] Warning: dummy plugin shouldn't be used as is!",ccMainAppInterface::WRN_CONSOLE_MESSAGE); //a warning message is displayed in the console
	m_app->dispToConsole("Dummy plugin shouldn't be used as is!",ccMainAppInterface::ERR_CONSOLE_MESSAGE); //an error message is displayed in the console AND an error box will pop-up!*/
}

/* Called when the plugin is being stopped */
void ccExametrics::stop()
{
	/*//remove click listener
	if (m_app->getActiveGLWindow())
	{
		m_app->getActiveGLWindow()->removeEventFilter(this);
	}*/

	//remove overlay GUI
	if (m_dlg)
	{
		m_dlg->stop(true);
		m_app->unregisterOverlayDialog(m_dlg);
	}

	//redraw
	if (m_app->getActiveGLWindow())
	{
		m_app->getActiveGLWindow()->redraw(true, false);
	}

	m_dlg = nullptr;
	ccStdPluginInterface::stop();
}

/* This method should return the plugin icon (it will be used mainly
if your plugin as several actions in which case CC will create a
dedicated sub-menu entry with this icon. */
QIcon ccExametrics::getIcon() const
{
	return QIcon(":/CC/plugin/qExametrics/exametrics_icon.png");
}

/* Slot on Compute button click */
void ccExametrics::onCompute()
{
	m_app->dispToConsole("[ccExametrics] Compute!",ccMainAppInterface::STD_CONSOLE_MESSAGE);

	// vector point is on the normalized vector ?
	if(!pointIsOnVector())
		return;


	//get point cloud
	ccPointCloud* cloud = static_cast<ccPointCloud*>(m_app->dbRootObject()); //cast to point cloud

	if (!cloud)
	{
		m_app->dispToConsole("[ccExametrics] not a cloud :(", ccMainAppInterface::WRN_CONSOLE_MESSAGE);
		return;
	}
	else
	{
		m_app->dispToConsole("[ccExametrics] is a cloud :)", ccMainAppInterface::STD_CONSOLE_MESSAGE);

	}
}

/* Slot on dialog closed */
void ccExametrics::onClose()
{
	stop();

	m_app->removeFromDB(this->normalizedVectorPoly);
	m_app->removeFromDB(this->vectorPoint2DLabel);
	m_app->removeFromDB(this->plan);
}

/* Initialize plan parameters at random values with min and max limits */
void ccExametrics::initializeSpinBox(ccBBox box)
{
	CCVector3 minCorner = box.minCorner();
	CCVector3 maxCorner = box.maxCorner();

	float xBox = 0, yBox = 0, zBox = 0;
	xBox = maxCorner.x - minCorner.x;
	yBox = maxCorner.y - minCorner.y;
	zBox = maxCorner.z - minCorner.z;

	this->m_boxXWidth = xBox;
	this->m_boxYWidth = yBox;

	m_dlg->spbXA->setMinimum(minCorner.x);
	m_dlg->spbYA->setMinimum(minCorner.y);
	m_dlg->spbZA->setMinimum(minCorner.z);
	m_dlg->spbXA->setMaximum(maxCorner.x);
	m_dlg->spbYA->setMaximum(maxCorner.y);
	m_dlg->spbZA->setMaximum(maxCorner.z);

	m_dlg->spbXB->setMinimum(minCorner.x);
	m_dlg->spbYB->setMinimum(minCorner.y);
	m_dlg->spbZB->setMinimum(minCorner.z);
	m_dlg->spbXB->setMaximum(maxCorner.x);
	m_dlg->spbYB->setMaximum(maxCorner.y);
	m_dlg->spbZB->setMaximum(maxCorner.z);

	m_dlg->spbX->setMinimum(minCorner.x);
	m_dlg->spbY->setMinimum(minCorner.y);
	m_dlg->spbZ->setMinimum(minCorner.z);
	m_dlg->spbX->setMaximum(maxCorner.x);
	m_dlg->spbY->setMaximum(maxCorner.y);
	m_dlg->spbZ->setMaximum(maxCorner.z);

	// random vector
	m_dlg->spbXA->setValue(frand_a_b(minCorner.x, maxCorner.x));
	m_dlg->spbYA->setValue(frand_a_b(minCorner.x, maxCorner.x));
	m_dlg->spbZA->setValue(frand_a_b(minCorner.x, maxCorner.x));
	m_dlg->spbXB->setValue(frand_a_b(minCorner.x, maxCorner.x));
	m_dlg->spbYB->setValue(frand_a_b(minCorner.x, maxCorner.x));
	m_dlg->spbZB->setValue(frand_a_b(minCorner.x, maxCorner.x));

	// random point on vector [(Xb - Xa) * k + Xa]
	double k = frand_a_b(0, 1);
	double x = (m_dlg->spbXB->value() - m_dlg->spbXA->value()) * k + m_dlg->spbXA->value();
	double y = (m_dlg->spbYB->value() - m_dlg->spbYA->value()) * k + m_dlg->spbYA->value();
	double z = (m_dlg->spbZB->value() - m_dlg->spbZA->value()) * k + m_dlg->spbZA->value();
	m_dlg->spbX->setValue(x);
	m_dlg->spbY->setValue(y);
	m_dlg->spbZ->setValue(z);

	// tolerance
	m_dlg->toleranceSpb->setValue(0.01);


	QString boundariesString = "[ccExametrics] Boundaries: ("
								+ QString::number(xBox) + "; "
								+ QString::number(yBox) + "; "
								+ QString::number(zBox) + ")";

	m_app->dispToConsole(boundariesString, ccMainAppInterface::STD_CONSOLE_MESSAGE);
}

/* Initialize draw settings for normalized vector, point and plan display */
void ccExametrics::initializeDrawSettings()
{
	/* Normalized vector */

	// new 2 points cloud
	this->normalizedVectorCloud = new ccPointCloud("Normalized vector");

	// reserve 2 points
	this->normalizedVectorCloud->reserve(2);

	// add points
	CCVector3 vec0(m_dlg->spbXA->value(), m_dlg->spbYA->value(), m_dlg->spbZA->value());
	CCVector3 vec1(m_dlg->spbXB->value(), m_dlg->spbYB->value(), m_dlg->spbZB->value());
	this->normalizedVectorCloud->addPoint(vec0);
	this->normalizedVectorCloud->addPoint(vec1);

	// create the polyline with the 2 points cloud
	this->normalizedVectorPoly = new ccPolyline(this->normalizedVectorCloud);
	// connect the two first points of the cloud
	this->normalizedVectorPoly->addPointIndex(0,2);
	// color
	ccColor::Rgb lineColor = ccColor::red ;
	this->normalizedVectorPoly->setColor(lineColor);
	this->normalizedVectorPoly->showColors(true);
	// where to display
	this->normalizedVectorPoly->setDisplay(m_app->getActiveGLWindow());
	// save in DB tree
	m_app->addToDB(this->normalizedVectorPoly);

	/* Vector point */

	this->vectorPointCloud = new ccPointCloud("Vector point");
	this->vectorPointCloud->reserve(1);
	this->vectorPointCloud->addPoint(CCVector3(m_dlg->spbX->value(), m_dlg->spbY->value(), m_dlg->spbZ->value()));
	this->vectorPoint2DLabel = new cc2DLabel("Vector point");
	this->vectorPoint2DLabel->addPoint(this->vectorPointCloud, this->vectorPointCloud->size() - 1);
	this->vectorPoint2DLabel->setVisible(true);
	this->vectorPoint2DLabel->setDisplay(m_app->getActiveGLWindow());
	m_app->addToDB(this->vectorPoint2DLabel);

	/* Plan */
	this->planCloud = new ccPointCloud("Plan");
	this->planCloud->reserve(4);
	planTransformation = new ccGLMatrix();
	planTransformation->toIdentity();
	/*planTransformation->scaleLine(0, this->m_boxXWidth);
	planTransformation->scaleLine(1, this->m_boxYWidth);*/

	CCVector3 normalizedVector = computeVector();
	CCVector3 vectorPoint = CCVector3(m_dlg->spbX->value(), m_dlg->spbY->value(), m_dlg->spbZ->value());
	double normalizedVectorNorm = normalizedVector.normd();
	double normOB = sqrt(pow(m_dlg->spbXB->value(), 2) + pow(m_dlg->spbYB->value(), 2) + pow(m_dlg->spbZB->value(), 2));
	double phi = acos(normalizedVector.x / (normalizedVectorNorm * sin(acos(normalizedVector.z / normalizedVectorNorm)))); // (x)
	double theta = 0;
    double psi = acos(normalizedVector.z / normalizedVectorNorm); // (z)
	/*double phi = acos(normalizedVector.x / normalizedVectorNorm);
	double theta = acos(normalizedVector.y / normalizedVectorNorm);
	double psi = acos(normalizedVector.z / normalizedVectorNorm);*/
	m_app->dispToConsole("composantes x: " + QString::number(normalizedVector.x)
                        + " y: " + QString::number(normalizedVector.y)
                        + " z: " + QString::number(normalizedVector.z)
                        + "\nnorme: " + QString::number(normalizedVectorNorm)
                        + "\n normOB: " + QString::number(normOB));
	m_app->dispToConsole("Phi = " + QString::number(phi) + " or = " + QString::number(phi * 180 / M_PI), ccMainAppInterface::STD_CONSOLE_MESSAGE);
	m_app->dispToConsole("Theta = " + QString::number(theta) + " or = " + QString::number(theta * 180 / M_PI), ccMainAppInterface::STD_CONSOLE_MESSAGE);
	m_app->dispToConsole("Psi = " + QString::number(psi) + " or = " + QString::number(psi * 180 / M_PI), ccMainAppInterface::STD_CONSOLE_MESSAGE);

    //CCVector3 orthoVector1 = normalizedVector.orthogonal();
    //m_app->dispToConsole("[ccExametrics] ortho vector 1:  " + QString::number(orthoVector1.x) + " " + QString::number(orthoVector1.y) + " " + QString::number(orthoVector1.z));
	// initFromParameters phi theta psi: projeter le vecteur normal pour obtenir les angles + translation depuis le point
	//planTransformation->initFromParameters(phi, theta, psi, Vector3Tpl<float>(0,0,0));
    planTransformation->initFromParameters(phi, theta, psi, vectorPoint);
	//planTransformation->initFromParameters(0,0,0,vectorPoint);

    //planTransformation->applyRotation(orthoVector1);
	this->plan = new ccPlane(m_boxXWidth, m_boxYWidth, planTransformation);
	this->plan->setDisplay(m_app->getActiveGLWindow());
	m_app->addToDB(this->plan);
}

void ccExametrics::onSpbXAChanged(double value){ onParameterChanged(m_dlg->spbXA, value); onNormalizedVectorChanged(); }
void ccExametrics::onSpbYAChanged(double value){ onParameterChanged(m_dlg->spbYA, value); onNormalizedVectorChanged(); }
void ccExametrics::onSpbZAChanged(double value){ onParameterChanged(m_dlg->spbZA, value); onNormalizedVectorChanged(); }

void ccExametrics::onSpbXBChanged(double value){ onParameterChanged(m_dlg->spbXB, value); onNormalizedVectorChanged(); }
void ccExametrics::onSpbYBChanged(double value){ onParameterChanged(m_dlg->spbYB, value); onNormalizedVectorChanged(); }
void ccExametrics::onSpbZBChanged(double value){ onParameterChanged(m_dlg->spbZB, value); onNormalizedVectorChanged(); }

void ccExametrics::onSpbXChanged(double value){ onParameterChanged(m_dlg->spbX, value); onVectorPointChanged(); }
void ccExametrics::onSpbYChanged(double value){ onParameterChanged(m_dlg->spbY, value); onVectorPointChanged(); }
void ccExametrics::onSpbZChanged(double value){ onParameterChanged(m_dlg->spbZ, value); onVectorPointChanged(); }

void ccExametrics::onToleranceSpbChanged(double value){ onParameterChanged(m_dlg->toleranceSpb, value); }

/* Called when the parameters of the normalized vector are changing */
void ccExametrics::onNormalizedVectorChanged()
{
	if(!this->normalizedVectorCloud)
		return;

	// clear old points
	this->normalizedVectorCloud->clear();

	// reserve 2 points
	this->normalizedVectorCloud->reserve(2);

	// add points
	CCVector3 vec0(m_dlg->spbXA->value(), m_dlg->spbYA->value(), m_dlg->spbZA->value());
	CCVector3 vec1(m_dlg->spbXB->value(), m_dlg->spbYB->value(), m_dlg->spbZB->value());
	this->normalizedVectorCloud->addPoint(vec0);
	this->normalizedVectorCloud->addPoint(vec1);

	//m_app->getActiveGLWindow()->moveCamera(1,0,0);
	//m_app->updateUI();
	//m_app->refreshAll();
}

void ccExametrics::onVectorPointChanged()
{
	if(!this->vectorPointCloud)
		return;

	// verif point est sur le vecteur (doit verifier [(Xb - Xa) * k + Xa])
	if(!pointIsOnVector())
		m_app->dispToConsole("[ccExametrics] Defined point is not on the normalized vector.", ccMainAppInterface::WRN_CONSOLE_MESSAGE);

	this->vectorPointCloud->clear();
	this->vectorPointCloud->reserve(1);
	this->vectorPointCloud->addPoint(CCVector3(m_dlg->spbX->value(), m_dlg->spbY->value(), m_dlg->spbZ->value()));
}

void ccExametrics::onParameterChanged(QDoubleSpinBox* spb, double value)
{
	//m_app->dispToConsole("[ccExametrics] onParameterChanged " + spb->objectName() + " " + QString::number(value), ccMainAppInterface::STD_CONSOLE_MESSAGE);
}


double ccExametrics::getNormX()
{
    return abs(m_dlg->spbXB->value() - m_dlg->spbXA->value());
}

double ccExametrics::getNormY()
{
    return abs(m_dlg->spbYB->value() - m_dlg->spbYA->value());
}

double ccExametrics::getNormZ()
{
    return abs(m_dlg->spbZB->value() - m_dlg->spbZA->value());
}

CCVector3 ccExametrics::computeVector()
{
	return CCVector3(getNormX(), getNormY(), getNormZ());
}

bool ccExametrics::pointIsOnVector()
{
	double kx = 0, ky = 0, kz = 0;
	bool isOn = false;

	kx = (m_dlg->spbX->value() - m_dlg->spbXA->value()) / (m_dlg->spbXB->value() - m_dlg->spbXA->value());
	ky = (m_dlg->spbY->value() - m_dlg->spbYA->value()) / (m_dlg->spbYB->value() - m_dlg->spbYA->value());
	kz = (m_dlg->spbZ->value() - m_dlg->spbZA->value()) / (m_dlg->spbZB->value() - m_dlg->spbZA->value());

	if(double_equals(kx, ky) && double_equals(kx, kz) && double_equals(ky, kz))
		isOn = true;

	/*m_app->dispToConsole("[ccExametrics] kx = " + QString::number(kx)
						+ " ky = " + QString::number(ky)
						+ " kz = " + QString::number(kz)
						+ " isOn = " + QString::number(isOn)
						, ccMainAppInterface::STD_CONSOLE_MESSAGE);*/

	return isOn;
}

bool ccExametrics::double_equals(double a, double b, double epsilon)
{
    return std::abs(a - b) < epsilon;
}

double ccExametrics::frand_a_b(double a, double b)
{
    return ( rand()/(double)RAND_MAX ) * (b-a) + a;
}
