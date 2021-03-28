#include <string>
#include <sstream>

using namespace std;

class Version {

private:
	string _versionString;

	void Parse(int result[4], const std::string& input);
	bool LessThanVersion(const std::string& a, const std::string& b);
public:
	string to_string() {
		return _versionString;
	}

	bool operator<(Version v2) {
		return LessThanVersion(_versionString, v2.to_string());
	}

	Version(string versionString) {
		_versionString = versionString;
	}
};