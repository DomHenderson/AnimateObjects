#include <chrono>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <string>
#include <vector>

std::default_random_engine Generator(std::chrono::system_clock::now().time_since_epoch().count());

enum class Stat {
	Str, Dex, Con, Int, Wis, Cha
};

enum class RollType {
	Standard, Advantage, Disadvantage
};

unsigned d20Roll(RollType rollType) {
	std::uniform_int_distribution<unsigned> die(1, 20);
	unsigned roll = die(Generator);

	if(rollType == RollType::Standard) {
		return roll;
	} else {
		std::uniform_int_distribution<unsigned> otherDie(1, 20);
		unsigned otherRoll = otherDie(Generator);
		return rollType == RollType::Advantage
			? std::max(roll, otherRoll)
			: std::min(roll, otherRoll);
	}
}

class AttackResult {
public:
	AttackResult(unsigned toHit, unsigned damage): toHit(toHit), critSuccess(false), critFail(false), damage(damage) {}
	int toHit;
	bool critSuccess;
	bool critFail;
	unsigned damage;
protected:
	AttackResult(unsigned toHit, bool critSuccess, bool critFail, unsigned damage):
		toHit(toHit), critSuccess(critSuccess), critFail(critFail), damage(damage) {}
};

class CriticalHit: public AttackResult {
public:
	CriticalHit(unsigned damage): AttackResult(std::numeric_limits<int>::max(), true, false, damage) {}
};

class CriticalMiss: public AttackResult {
public:
	CriticalMiss(): AttackResult(std::numeric_limits<int>::min(), false, true, 0) {}
};

struct AnimatedObject {
	bool CheckAgainstAC(unsigned toHit) const {
		return toHit >= AC;
	}

	void ApplyDamage(unsigned damage) {
		HP = (damage>HP) ? 0 : HP-damage;
	}

	int RollStat(Stat stat, RollType rollType) {
		int saveStat = stats[stat];
		int saveMod = (saveStat/2)-5;
		int dieRoll = d20Roll(rollType);
		return dieRoll+saveMod;
	}

	AttackResult RollAttack(RollType rollType) {
		const unsigned roll = d20Roll(rollType);

		if (roll == 1) {
			return CriticalMiss();
		} else {
			std::uniform_int_distribution<unsigned> die(1, attackDieSize);

			const unsigned dieCount = ((roll == 20) ? 2 : 1) * attackDieCount;
			unsigned totalDamage = attackFlatDamage;
			for(unsigned i = 0; i < dieCount; ++i) {
				totalDamage += die(Generator);
			}

			if(roll == 20) {
				return CriticalHit(totalDamage);
			} else {
				unsigned toHit = roll+attackMod;
				return AttackResult(toHit, totalDamage);
			}
		}
	}

	unsigned getHP() {
		return HP;
	}

protected:
	unsigned HP;
	unsigned AC;
	std::unordered_map<Stat,unsigned> stats;
	unsigned attackMod;
	unsigned attackDieSize;
	unsigned attackDieCount;
	unsigned attackFlatDamage;
	unsigned effectiveObjectCount;
	std::unordered_set<std::string> conditions;
};

struct TinyAO: public AnimatedObject {
	TinyAO() {
		HP = 20;
		AC = 18;
		stats[Stat::Str] = 4;
		stats[Stat::Dex] = 18;
		stats[Stat::Con] = 10;
		stats[Stat::Wis] = 3;
		stats[Stat::Int] = 3;
		stats[Stat::Cha] = 1;
		attackMod = 8;
		attackDieCount = 1;
		attackDieSize = 4;
		attackFlatDamage = 4;
		effectiveObjectCount = 1;
	}
};

class AOCollection {
public:
	AOCollection():
		objects(10, TinyAO())
	{}
	void PrintStatus() {
		std::cout<<"Num left: "<<objects.size()<<"\n";
		std::cout<<"HPs: ";
		for(auto& AO: objects) {
			std::cout<<AO.getHP()<<",";
		}
		std::cout<<std::endl;
	}
	void ApplyAttack(unsigned toHit, unsigned damage) {
		if(objects.size() == 0) {
			std::cout<<"No objects to take damage"<<std::endl;
			return;
		}
		const unsigned i = objects.size() - 1;
		if(objects[i].CheckAgainstAC(toHit)) {
			objects[i].ApplyDamage(damage);
			if(objects[i].getHP() == 0) {
				objects.pop_back();
			}
		}
	}
	void Attack(RollType rollType) {
		if(objects.size() == 0) {
			std::cout<<"No objects with which to attack"<<std::endl;
			return;
		}
		std::vector<AttackResult> attackResults;
		std::transform(objects.begin(), objects.end(), std::back_inserter(attackResults), 
			[rollType](AnimatedObject result) {return result.RollAttack(rollType);}
		);
		std::sort(attackResults.begin(), attackResults.end(),
			[](AttackResult left, AttackResult right) { //should return true iff left < right
				if(left.critFail) {
					return !right.critFail;
				} else if (left.critSuccess) {
					if(right.critSuccess) {
						return left.damage < right.damage;
					} else {
						return false;
					}
				} else {
					if(left.toHit == right.toHit) {
						return left.damage < right.damage;
					} else {
						return left.toHit < right.toHit;
					}
				}
			}
		);

		unsigned maxTotalDamage = 0;
		std::for_each(attackResults.begin(), attackResults.end(),
			[&maxTotalDamage](const AttackResult& result) {
				maxTotalDamage += result.damage;
			}
		);

		int previousToHit = -1;
		std::for_each(attackResults.begin(), attackResults.end(),
			[&maxTotalDamage, &previousToHit](const AttackResult& result) {
				if(result.critFail) {
					std::cout<<"Critical Miss!";
				} else if(result.critSuccess) {
					std::cout<<"Critical hit! ";
					std::cout<<result.damage<<" damage ";
					std::cout<<"("<<maxTotalDamage<<" total)";
				} else {
					std::cout<<result.toHit<<" to hit ";
					std::cout<<result.damage<<" damage ";
					if(result.toHit != previousToHit) {
						std::cout<<"("<<maxTotalDamage<<" total)";
						previousToHit = result.toHit;
					}
					maxTotalDamage -= result.damage;
				}
				std::cout<<std::endl;
			}
		);
	}

	void Save(Stat stat, bool halfIfSave, int DC, int damage, RollType rollType) {
		std::cout<<"Running save DC: "<<DC<<" or "<<damage<<" damage\n";
		for(unsigned i = 0; i < objects.size(); ++i) {

			int saveAttempt = objects[i].RollStat(stat, rollType);
			bool saveSuccess = (saveAttempt >= DC);
			unsigned damageToApply = saveSuccess
				? (halfIfSave ? damage/2: 0)
				: damage;

			std::cout<<(saveSuccess ? "Saved!" : "Failed");
			std::cout<<" ("<<saveAttempt;
			std::cout<<") HP: "<<objects[i].getHP()<<" -> ";

			objects[i].ApplyDamage(damageToApply);
			std::cout<<objects[i].getHP()<<"\n";
			if(objects[i].getHP() == 0) {
				objects.erase(objects.begin()+i);
				--i;
			}
		}
		std::cout<<"----"<<std::endl;
	}
private:
	std::vector<AnimatedObject> objects;
};

std::vector<std::string> parseCommand(std::string input) {
	std::vector<std::string> result;
	size_t index = input.find(" ");
	while(index != std::string::npos) {
		std::string word = input.substr(0, index);
		result.push_back(word);
		input = input.substr(index+1);
		index = input.find(" ");
	}
	result.push_back(input);

	return result;
}

int main() {
	AOCollection AOs;

	std::unordered_map<std::string, Stat> statStrings;
	statStrings.emplace("str", Stat::Str);
	statStrings.emplace("dex", Stat::Dex);
	statStrings.emplace("con", Stat::Con);
	statStrings.emplace("int", Stat::Int);
	statStrings.emplace("wis", Stat::Wis);
	statStrings.emplace("cha", Stat::Cha);

	while(true) {
		std::string input;
		std::getline(std::cin, input);

		std::vector<std::string> parsedCommand = parseCommand(input);
		for(unsigned i = 0; i <parsedCommand.size(); ++i) {
			std::cout<<"'"<<parsedCommand[i]<<"'"<<std::endl;
		}

		auto command = statStrings.find(parsedCommand[0]);
		if(command != statStrings.end()) {
			std::cout<<"Found stat"<<std::endl;
			RollType rollType;
			if(parsedCommand.size() >= 5) {
				if(parsedCommand[4] == "adv") {
					rollType = RollType::Advantage;
				} else if (parsedCommand[4] == "disadv") {
					rollType = RollType::Disadvantage;
				} else {
					std::cout<<parsedCommand[4]<<" not recognised"<<std::endl;
					break;
				}
			} else {
				rollType = RollType::Standard;
			}
			if(parsedCommand[1] == "half") {
				std::cout<<"Running save"<<std::endl;
				AOs.Save(command->second, true, std::stoi(parsedCommand[2]), std::stoi(parsedCommand[3]), rollType);
			} else if (parsedCommand[1] == "none") {
				std::cout<<"Running save"<<std::endl;
				AOs.Save(command->second, false, std::stoi(parsedCommand[2]), std::stoi(parsedCommand[3]), rollType);
			} else {
				std::cout<<"Invalid syntax"<<std::endl;
			}
		} else if (parsedCommand[0] == "attack") {
			RollType rollType;
			if(parsedCommand.size() >= 2) {
				if(parsedCommand[1] == "adv") {
					rollType = RollType::Advantage;
				} else if (parsedCommand[1] == "disadv") {
					rollType = RollType::Disadvantage;
				} else {
					std::cout<<parsedCommand[1]<<" not recognised"<<std::endl;
					break;
				}
			} else {
				rollType = RollType::Standard;
			}
			AOs.Attack(rollType);
		} else if (parsedCommand[0] == "take") {
			AOs.ApplyAttack(std::stoi(parsedCommand[1]), std::stoi(parsedCommand[2]));
		} else if (parsedCommand[0] == "stop") {
			break;
		} else if (parsedCommand[0] == "status") {
			AOs.PrintStatus();
		} else if (parsedCommand[0] == "help") {
			std::cout<<"Commands:\n";
			std::cout<<"[stat] [half/none] [dc] [damage] [adv/disadv?]\n";
			std::cout<<"attack [adv/disadv?]\n";
			std::cout<<"take [damage]\n";
			std::cout<<"status\n";
			std::cout<<"help\n";
			std::cout<<"stop\n";
			std::cout<<"---"<<std::endl;
		}
		std::cout<<"Next"<<std::endl;
	}
	return 0;
}