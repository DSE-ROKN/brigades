#include <memory>
#include <list>
#include <iostream>

#include "MilitaryUnit.h"
#include "MilitaryUnitAI.h"
#include "PlatoonAI.h"
#include "Army.h"
#include "Papaya.h"

Platoon::Platoon(MilitaryUnit* commandingunit, const Vector2& pos, ServiceBranch b, int side)
	: MilitaryUnit(commandingunit, b, side),
	mPosition(pos),
	mController(nullptr),
	mHealth(100.0f),
	mVisibilityCheckDelay(0.0f)
{
	mController = std::shared_ptr<Controller<Platoon>>(new PlatoonAIController(this));
}

ServiceBranch Platoon::getBranch() const
{
	return mBranch;
}

std::list<Platoon*> Platoon::update(float dt)
{
	if(isDead()) {
		return std::list<Platoon*>();
	}
	mVisibilityCheckDelay -= dt;
	if(mVisibilityCheckDelay <= 0.0f) {
		mVisibilityCheckDelay = 1.0f;
		checkVisibility();
	}
	if(mController->control(dt)) {
		return std::list<Platoon*>(1, this);
	}
	else {
		return std::list<Platoon*>();
	}
}

std::list<Platoon*> Platoon::getPlatoons()
{
	return std::list<Platoon*>(1, this);
}

int Platoon::getSide() const
{
	return mSide;
}

Vector2 Platoon::getPosition() const
{
	return mPosition;
}

void Platoon::setPosition(const Vector2& v)
{
	mPosition = v;
}

void Platoon::receiveMessage(const Message& m)
{
	mController->receiveMessage(m);
}

void Platoon::setController(std::shared_ptr<Controller<Platoon>> c)
{
	mController = c;
}

void Platoon::checkVisibility()
{
	for(Platoon* p = Papaya::instance().getNeighbouringPlatoons(this, 4.0f);
			p != nullptr;
			p = Papaya::instance().getNextNeighbouringPlatoon()) {
		if(p->getSide() != getSide() && !p->isDead()) {
			MessageDispatcher::instance().dispatchMessage(Message(mEntityID, mEntityID,
						0.0f, 0.0f, MessageType::EnemyDiscovered, p));
		}
	}
}

UnitSize Platoon::getUnitSize() const
{
	return UnitSize::Platoon;
}

UnitSize Company::getUnitSize() const
{
	return UnitSize::Company;
}

UnitSize Battalion::getUnitSize() const
{
	return UnitSize::Battalion;
}

UnitSize Brigade::getUnitSize() const
{
	return UnitSize::Brigade;
}

UnitSize Army::getUnitSize() const
{
	return UnitSize::Division;
}

void Platoon::moveTowards(const Vector2& v, float dt)
{
	Vector2 velvec;
	if(v.length() > 1.0f)
		velvec = v.normalized();
	else
		velvec = v;
	velvec *= 0.1f * dt * Papaya::instance().getPlatoonSpeed(*this);
	velvec += getPosition();
	Vector2 oldpos = getPosition();
	setPosition(velvec);
	Papaya::instance().updateEntityPosition(this, oldpos);
}

void Platoon::loseHealth(float damage)
{
	bool wasdead = isDead();
	mHealth -= damage;
	if(!wasdead && isDead()) {
		MessageDispatcher::instance().dispatchMessage(Message(mEntityID, WORLD_ENTITY_ID,
					0.0f, 0.0f, MessageType::PlatoonDied, this));
	}
}

bool Platoon::isDead() const
{
	return mHealth <= 0.0f;
}

bool MilitaryUnit::isDead() const
{
	for(auto& p : mUnits) {
		if(!p->isDead())
			return false;
	}
	return true;
}

float MilitaryUnit::getHealth() const
{
	float h = 0.0f;
	for(auto& p : mUnits) {
		h += p->getHealth();
	}
	return h;
}

float Platoon::getHealth() const
{
	return std::max(0.0f, mHealth);
}

MilitaryUnit::MilitaryUnit(MilitaryUnit* commandingunit, ServiceBranch b, int side)
	: mCommandingUnit(commandingunit),
	mBranch(b),
	mSide(side),
	mController(nullptr)
{
	mController = std::shared_ptr<Controller<MilitaryUnit>>(new MilitaryUnitAIController(this));
}

const MilitaryUnit* MilitaryUnit::getCommandingUnit() const
{
	return mCommandingUnit;
}

MilitaryUnit* MilitaryUnit::getCommandingUnit()
{
	return mCommandingUnit;
}

const std::vector<std::shared_ptr<MilitaryUnit>>& MilitaryUnit::getUnits() const
{
	return mUnits;
}

std::vector<std::shared_ptr<MilitaryUnit>>& MilitaryUnit::getUnits()
{
	return mUnits;
}

void MilitaryUnit::receiveMessage(const Message& m)
{
	mController->receiveMessage(m);
}

ServiceBranch MilitaryUnit::getBranch() const
{
	return mBranch;
}

int MilitaryUnit::getSide() const
{
	return mSide;
}

std::list<Platoon*> MilitaryUnit::getPlatoons()
{
	std::list<Platoon*> units;
	for(auto& u : mUnits) {
		units.splice(units.end(), u->getPlatoons());
	}
	return units;
}

std::list<Platoon*> MilitaryUnit::update(float dt)
{
	std::list<Platoon*> units;
	for(auto& u : mUnits) {
		units.splice(units.end(), u->update(dt));
	}
	return units;
}

Vector2 MilitaryUnit::getPosition() const
{
	Vector2 pos;
	for(auto& u : mUnits) {
		pos += u->getPosition();
	}
	if(mUnits.size())
		pos *= 1.0f / mUnits.size();
	return pos;
}

float MilitaryUnit::distanceTo(const MilitaryUnit& m) const
{
	Vector2 diff = getPosition() - m.getPosition();
	return diff.length();
}

Company::Company(MilitaryUnit* commandingunit, const Vector2& pos, ServiceBranch b, int side)
	: MilitaryUnit(commandingunit, b, side)
{
	for(int i = 0; i < 4; i++) {
		std::shared_ptr<Platoon> p(new Platoon(this, pos + spawnUnitDisplacement(), mBranch, mSide));
		mUnits.push_back(p);
		Papaya::instance().addEntityPosition(p.get());
	}
}

Battalion::Battalion(MilitaryUnit* commandingunit, const Vector2& pos, ServiceBranch b, int side)
	: MilitaryUnit(commandingunit, b, side)
{
	for(int i = 0; i < 4; i++) {
		mUnits.push_back(std::shared_ptr<Company>(new Company(this, pos + spawnUnitDisplacement(), mBranch, mSide)));
	}
}

Brigade::Brigade(MilitaryUnit* commandingunit, const Vector2& pos, ServiceBranch b, int side, const std::vector<ServiceBranch>& config)
	: MilitaryUnit(commandingunit, b, side)
{
	for(auto& br : config) {
		mUnits.push_back(std::shared_ptr<Battalion>(new Battalion(this, pos + spawnUnitDisplacement(), br, mSide)));
	}
}

Vector2 MilitaryUnit::spawnUnitDisplacement() const
{
	float add = 0.002f;
	switch(getUnitSize()) {
		case UnitSize::Division:
			add *= 4;
		case UnitSize::Brigade:
			add *= 2;
		case UnitSize::Battalion:
			add *= 4;
		case UnitSize::Company:
			add *= 4;
		case UnitSize::Platoon:
			add *= 8;
		case UnitSize::Squad:
			add *= 8;
		case UnitSize::Single:
			break;
	}

	int num = mUnits.size();
	if(getSide() % 2 == 0)
		num = -num;
	return Vector2(num % 2 * add, num / 2 * add);
}

void MilitaryUnit::setController(std::shared_ptr<Controller<MilitaryUnit>> c)
{
	mController = c;
}


