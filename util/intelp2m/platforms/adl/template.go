package adl

import "../common"

type InheritanceTemplate interface {
	KeywordCheck(line string) bool
}

// GroupNameExtract - This function extracts the group ID, if it exists in a row
// line      : string from the configuration file
// return
//     bool   : true if the string contains a group identifier
//     string : group identifier
func (PlatformSpecific) GroupNameExtract(line string) (bool, string) {
	return common.KeywordsCheck(line,
		"GPP_B", "GPP_T", "GPP_A", "GPP_S", "GPP_I", "GPP_H", "GPP_D", "GPD",
		"GPP_C", "GPP_F", "GPP_E", "GPP_R", "GPP_J", "GPP_K", "GPP_G")
}

// KeywordCheck - This function is used to filter parsed lines of the configuration file and
//                returns true if the keyword is contained in the line.
// line      : string from the configuration file
func (platform PlatformSpecific) KeywordCheck(line string) bool {
	return platform.InheritanceTemplate.KeywordCheck(line)
}
