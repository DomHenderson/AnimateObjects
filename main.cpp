#include <chrono>
#include <iostream>
#include <random>
#include <vector>

struct AnimatedObject {
	unsigned HP;
	unsigned AC;
	unsigned Str;
	unsigned Dex;
	unsigned AttackMod;
	unsigned AttackDieSize;
	unsigned AttackDieCount;
	unsigned AttackFlatDamage;
	unsigned EffectiveObjectCount;
};

struct TinyAO: public AnimatedObject {
	TinyAO() {
		HP = 20;
		AC = 18;
		Str = 4;
		Dex = 18;
		AttackMod = 8;
		AttackDieCount = 1;
		AttackDieSize = 4;
		AttackFlatDamage = 4;
		EffectiveObjectCount = 1;
	}
};

class AOCollection {
public:
	AOCollection():
		objects(10, TinyAO()),
		generator(std::chrono::system_clock::now().time_since_epoch().count())
	{}
	void PrintStatus() {
		std::cout<<"Num left: "<<objects.size()<<"\n";
		std::cout<<"HPs: ";
		for(auto& AO: objects) {
			std::cout<<AO.HP<<",";
		}
		std::cout<<std::endl;
	}
	void ApplyAttack(unsigned Damage) {
		if(objects.size() == 0) {
			std::cout<<"No objects to take damage"<<std::endl;
			return;
		}
		const unsigned i = objects.size() - 1;
		if(Damage >= objects[i].HP) {
			objects.pop_back();
		} else {
			objects[i].HP -= Damage;
		}
	}
	void Attack() {
		if(objects.size() == 0) {
			std::cout<<"No objects with which to attack"<<std::endl;
			return;
		}
		std::vector<std::pair<int, int>> nonCrits;
		std::vector<int> crits;
		for(auto& object: objects) {
			std::uniform_int_distribution<unsigned> toHitDie(1, 20);
			std::uniform_int_distribution<unsigned> damageDie(1, object.AttackDieSize);
			unsigned dieRoll = toHitDie(generator);
			unsigned toHit = dieRoll+object.AttackMod;
			unsigned damage = object.AttackFlatDamage;
			unsigned dieCount = ((dieRoll == 20) ? 2 : 1)*object.AttackDieCount;
			for(unsigned i = 0; i < dieCount; ++i) {
				damage += damageDie(generator);
			}
			if(dieRoll == 20) {
				crits.push_back(damage);
			} else {
				nonCrits.push_back({toHit, damage});
			}
		}
		if(nonCrits.size() > 0) {
			std::sort(nonCrits.begin(), nonCrits.end());
			unsigned total = 0;
			for(unsigned i = 0; i < nonCrits.size(); ++i) {
				unsigned total = 0;
				for(unsigned j = i; j < nonCrits.size(); ++j) {
					total += nonCrits[j].second;
				}
				for(auto& crit: crits) {
					total += crit;
				}

				std::cout<<nonCrits[i].first<<" to hit,    ";
				std::cout<<nonCrits[i].second<<" damage    ";
				if(i == 0 || nonCrits[i].first != nonCrits[i-1].first) {
					std::cout<<total<<" total damage";
				}
				std::cout<<std::endl;
			}
		}
		if(crits.size() > 0) {
			unsigned total = 0;
			for(auto& result: crits) {
				total += result;
			}
			for(auto& result: crits) {
				std::cout<<"nat 20 to hit,    "
				        <<result<<" damage    "
				        <<total<<" total damage"
				        <<std::endl;
			}
		}
	}
private:
	std::vector<AnimatedObject> objects;

	std::default_random_engine generator;
};

int main() {
	AOCollection AOs;
	AOs.ApplyAttack(30);
	AOs.ApplyAttack(10);
	AOs.ApplyAttack(10);
	AOs.ApplyAttack(10);
	AOs.Attack();
	return 0;
}