#pragma once

#include "array"
#include "chai3d.h"
#include "Config.h"
#include "mutex"
#include "atomic"
#include "haptic_db_ffi.h"

using namespace std;
using namespace chai3d;

inline cMesh* createWall()
{
	auto wall = new cMesh();

	cVector3d edges[4];
	edges[0].set(0.00, 0.000, 0.5);
	edges[1].set(0.03, 1.2, 0.5);
	edges[2].set(0.03, 1.2, -0.5);
	edges[3].set(0.03, 0.000, -0.5);

	wall->newTriangle(edges[3], edges[1], edges[0]);
	wall->newTriangle(edges[3], edges[2], edges[1]);

	return wall;
}

class TargetChallenge
{
private:
	mutex m_targetMu;
	cShapeEllipsoid* m_target;
	int m_step = 0;
	cLabel* m_countLabel;

	vector<double> m_velocities;
	vector<double> m_leftTargets;
	vector<double> m_rightTargets;

	DB* m_db;

	int m_state = 0;
public:
	TargetChallenge(cWorld* world, DB* db) : m_db(db)
	{
		m_target = new cShapeEllipsoid(0.006, 0.006, 0.006);
		m_target->setLocalPos(0, 0.0, 0);
		m_target->setShowEnabled(false);
		m_target->m_material->setRed();
		world->addChild(m_target);

		m_velocities.push_back(0.0005);
		m_velocities.push_back(0.001);
		m_velocities.push_back(0.0007);

		m_leftTargets.push_back(-0.1);
		m_leftTargets.push_back(-0.15);
		m_leftTargets.push_back(-0.2);

		m_rightTargets.push_back(-0.06);
		m_rightTargets.push_back(-0.09);
		m_rightTargets.push_back(-0.08);
	}

	void init(cLabel* countLabel, cVector3d& springPos)
	{
		unique_lock<mutex> lock(m_targetMu);

		random_shuffle(m_velocities.begin(), m_velocities.end());
		random_shuffle(m_leftTargets.begin(), m_leftTargets.end());
		random_shuffle(m_rightTargets.begin(), m_rightTargets.end());
		
		m_countLabel = countLabel;
		m_step = 0;
		auto newPos = springPos;
		newPos.y(-0.19);
		newPos.z(newPos.z() + 0.02);
		m_target->setLocalPos(newPos);
		m_target->setShowEnabled(true);
		m_countLabel->setText("3");
		m_state = 0;
	}

	bool update1()
	{
		if (m_step < 120) {
			if (m_step < 40)
				m_countLabel->setText("3");
			else if (m_step < 80)
				m_countLabel->setText("2");
			else
				m_countLabel->setText("1");
			m_step++;
			return false;
		}
		m_countLabel->setText("");

		auto pos = m_target->getLocalPos();

		const auto i = m_state / 2;
		const auto velocity = m_velocities[i];
		const auto moveToRight = m_state % 2;

		if (moveToRight)
		{
			const auto targetY = m_rightTargets[i];
			pos.y(pos.y() + velocity);
			if (pos.y() >= targetY)
			{
				m_state++;
			}
		}
		else
		{
			const auto targetY = m_leftTargets[i];
			pos.y(pos.y() - velocity);
			if (pos.y() <= targetY)
			{
				m_state++;
			}
		}
		m_target->setLocalPos(pos);
		double dbPos[3];
		for (auto i = 0; i < 3; i++)
			dbPos[i] = pos.get(i);

		if (m_state / 2 == m_leftTargets.size())
		{
			m_target->setShowEnabled(false);
			return true;
		}

		return false;
	}

	bool update2()
	{
		if (m_step < 120) {
			if (m_step < 40)
				m_countLabel->setText("3");
			else if (m_step < 80)
				m_countLabel->setText("2");
			else
				m_countLabel->setText("1");
			m_step++;
			return false;
		}
		m_countLabel->setText("");

		auto pos = m_target->getLocalPos();
		pos.y(pos.y() +  0.005 * sin(0.15 * double((m_step - 120))));
		m_target->setLocalPos(pos);

		if (m_step == 1000)
		{
			m_target->setShowEnabled(false);
			return true;
		}

		m_step++;
		return false;
	}

};

class Environment
{
private:
	int m_width;
	int m_height;
	
	cShapeEllipsoid* m_toolTip;

	TargetChallenge* m_targetChallenge;
	atomic<bool> m_targetChallengeActive = false;

	int64_t m_flashHold = 10;
	atomic<int64_t> m_count = 0;
	atomic<int64_t> m_flash = -m_flashHold;

	cWorld* m_world;
	cCamera* m_camera;
	cPositionalLight* m_light;
public:
	array<cShapeBox*, 5> m_springs;
	array<cLabel*, 5> m_rightInputs;
	array<cLabel*, 5> m_leftInputs;
	array<cLabel*, 5> m_middleInputs;
	array<cLabel*, 5> m_hiddenLabels;
	array<cLabel*, 5> m_visibleLabels;
	cLabel* m_bottomRightLabel;
	cLabel* m_bottomMiddleLabel;

	cLabel* m_feedbackLabel;

	Environment(const int width, const int height, DB* db)
		: m_width(width)
		, m_height(height)
	{
		m_world = new cWorld();

		m_world->m_backgroundColor.setBlack();
		m_camera = new cCamera(m_world);
		m_camera->set(cVector3d(0.5, 0.0, 0.0), // camera position (eye)
			cVector3d(0.0, 0.0, 0.0), // look at position (target)
			cVector3d(0.0, 0.0, 1.0)); // direction of the (up) vector

		m_camera->setClippingPlanes(0.01, 10.0);

		m_camera->setStereoMode(C_STEREO_DISABLED);

		m_camera->setStereoEyeSeparation(0.01);
		m_camera->setStereoFocalLength(0.5);

		m_camera->setMirrorVertical(false);

		const auto background = new cBackground();
		m_camera->m_backLayer->addChild(background);

		m_light = new cPositionalLight(m_world);
		m_world->addChild(m_light);
		m_light->setEnabled(true);

		cVector3d springPos(0, SPRING_Y, 0.15);
		cVector3d rightInputPos(1400, 870, 0);
		cVector3d middleInputPos(1170, 870, 0);
		cVector3d leftInputPos(870, 870, 0);
		cVector3d hiddenLabelPos(900, 890, 0);
		cVector3d visibleLabelPos(100, 890, 0);

		for (auto i = 0; i < 5; i++)
		{
			m_springs[i] = new cShapeBox(0.0, SPRING_LENGTH, SPRING_WIDTH);
			m_springs[i]->m_material->setWhite();
			m_springs[i]->setLocalPos(springPos);
			m_world->addChild(m_springs[i]);
			springPos.z(springPos.z() - 0.07);

			m_rightInputs[i] = new cLabel(NEW_CFONTCALIBRI18());
			m_rightInputs[i]->m_fontColor.setBlack();
			m_rightInputs[i]->setFontScale(4.0);
			m_rightInputs[i]->setLocalPos(rightInputPos);
			rightInputPos.y(rightInputPos.y() - 180);
			m_camera->m_frontLayer->addChild(m_rightInputs[i]);

			m_middleInputs[i] = new cLabel(NEW_CFONTCALIBRI18());
			m_middleInputs[i]->m_fontColor.setBlack();
			m_middleInputs[i]->setFontScale(4.0);
			m_middleInputs[i]->setLocalPos(middleInputPos);
			middleInputPos.y(middleInputPos.y() - 180);
			m_camera->m_frontLayer->addChild(m_middleInputs[i]);

			m_leftInputs[i] = new cLabel(NEW_CFONTCALIBRI18());
			m_leftInputs[i]->m_fontColor.setBlack();
			m_leftInputs[i]->setFontScale(4.0);
			m_leftInputs[i]->setLocalPos(leftInputPos);
			leftInputPos.y(leftInputPos.y() - 180);
			m_camera->m_frontLayer->addChild(m_leftInputs[i]);

			m_visibleLabels[i] = new cLabel(NEW_CFONTCALIBRI18());
			m_visibleLabels[i]->m_fontColor.setBlack();
			m_visibleLabels[i]->setFontScale(4.0);
			m_visibleLabels[i]->setLocalPos(visibleLabelPos);
			m_visibleLabels[i]->setText(std::to_string(5 - i));
			visibleLabelPos.y(visibleLabelPos.y() - 190);
			m_camera->m_frontLayer->addChild(m_visibleLabels[i]);

			m_hiddenLabels[i] = new cLabel(NEW_CFONTCALIBRI16());
			m_hiddenLabels[i]->m_fontColor.setBlack();
			m_hiddenLabels[i]->setFontScale(4.0);
			m_hiddenLabels[i]->setLocalPos(hiddenLabelPos);
			hiddenLabelPos.y(hiddenLabelPos.y() - 180);
			m_camera->m_frontLayer->addChild(m_hiddenLabels[i]);
		}

		m_bottomRightLabel = new cLabel(NEW_CFONTCALIBRI28());
		m_bottomRightLabel->m_fontColor.setBlack();
		m_bottomRightLabel->setFontScale(4.0);
		m_bottomRightLabel->setLocalPos(900, 0, 0);
		m_camera->m_frontLayer->addChild(m_bottomRightLabel);

		m_bottomMiddleLabel = new cLabel(NEW_CFONTCALIBRI28());
		m_bottomMiddleLabel->m_fontColor.setBlack();
		m_bottomMiddleLabel->setFontScale(4.0);
		m_bottomMiddleLabel->setLocalPos(1300, 0, 0);
		m_camera->m_frontLayer->addChild(m_bottomMiddleLabel);

		
		m_feedbackLabel = new cLabel(NEW_CFONTCALIBRI144());
		m_feedbackLabel->setFontScale(3);
		m_feedbackLabel->setLocalPos(900, 360, 0);
		m_feedbackLabel->m_fontColor.setRed();
		m_feedbackLabel->setText("+");
		m_camera->m_frontLayer->addChild(m_feedbackLabel);

		auto wall = createWall();
		m_world->addChild(wall);
		wall->setLocalPos(0, 0, 0);

		m_toolTip = new cShapeEllipsoid(0.01, 0.01, 0.01);
		m_world->addChild(m_toolTip);

		m_targetChallenge = new TargetChallenge(m_world, db);
	}

	~Environment()
	{
		delete m_world;
	}

	void springPosY(const int i, const double toolTipY)
	{
		const auto springY = SPRING_Y + max(0.0, toolTipY - SPRING_Y);
		auto springPos = m_springs[i]->getLocalPos();
		springPos.y(springY);
		m_springs[i]->setLocalPos(springPos);
	}

	void toolTipPos(const int i, const double y) const
	{
		auto newPos = m_springs[i]->getLocalPos();
		newPos.y(y - SPRING_LENGTH / 2 - 0.01);
		m_toolTip->setLocalPos(newPos);
	}

	void showSpring(const int i, const bool yes)
	{
		m_springs[i]->setShowEnabled(yes);
	}

	void springColor(const int i, cColorb& color)
	{
		m_springs[i]->m_material->setColor(color);
	}

	void rightInput(const int i, const string content)
	{
		m_rightInputs[i]->setText(content);
	}

	void middleInput(const int i, const string content)
	{
		m_middleInputs[i]->setText(content);
	}

	void leftInput(const int i, const string content)
	{
		m_leftInputs[i]->setText(content);
	}

	void visibleLabel(const int i, const string content)
	{
		m_visibleLabels[i]->setText(content);
	}

	void hiddenLabel(const int i, const string content)
	{
		m_hiddenLabels[i]->setText(content);
	}

	void bottomRightLabel(const string content) const
	{
		m_bottomRightLabel->setText(content);
	}

	void bottomMiddleLabel(const string content) const
	{
		m_bottomMiddleLabel->setText(content);
	}

	size_t numSprings() const
	{
		return m_springs.size();
	}

	void showTargetChallenge(const int i)
	{
		auto pos = m_springs[i]->getLocalPos();
		m_targetChallenge->init(m_hiddenLabels[i],pos);
		m_targetChallengeActive.store(true);
	}

	void flash(const string content)
	{
		m_feedbackLabel->setText(content);
		m_flash.store(m_count);
	}
	
	void updateGraphics()
	{
		if (m_targetChallengeActive.load() && m_targetChallenge->update2()) 
			m_targetChallengeActive.store(false);

		const auto count = m_count++;
		if (count < m_flash + m_flashHold)
			m_feedbackLabel->setShowEnabled(true);
		else
			m_feedbackLabel->setShowEnabled(false);

		m_world->updateShadowMaps(false, false);
		m_camera->renderView(m_width, m_height);

		GLenum err;
		err = glGetError();
		if (err != GL_NO_ERROR) cout << "Error:  %s\n" << gluErrorString(err);
	}
};
