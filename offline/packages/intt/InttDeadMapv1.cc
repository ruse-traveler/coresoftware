#include "InttDeadMapv1.h"

InttDeadMapv1::InttDeadMapv1(
) {
	// Do nothing
}

InttDeadMapv1::~InttDeadMapv1(
) {
	delete m_HotChannelSet;
}

void InttDeadMapv1::identify (
	std::ostream& out
) const {
	out << "InttDeadMapv1" << "\n"
	    << "\tsize: " << size() << std::endl;
}

std::size_t InttDeadMapv1::size (
) const {
	if(!m_HotChannelSet)return 0;
	return m_HotChannelSet->size();
}

int InttDeadMapv1::v_LoadFromCDBTTree (
	CDBTTree& cdbttree
) {
	if(!m_HotChannelSet)m_HotChannelSet = new Set_t;

	m_HotChannelSet->clear();
	Long64_t N = cdbttree.GetSingleIntValue("size");
	for(Long64_t n = 0; n < N; ++n) {
		m_HotChannelSet->insert((struct InttMap::Offline_s) {
			.layer          = cdbttree.GetIntValue(n, "layer"),
			.ladder_phi     = cdbttree.GetIntValue(n, "ladder_phi"),
			.ladder_z       = cdbttree.GetIntValue(n, "ladder_z"),
			.strip_phi      = cdbttree.GetIntValue(n, "strip_phi"),
			.strip_z        = cdbttree.GetIntValue(n, "strip_z"),
		});
	}

	return 0;
}

bool InttDeadMapv1::IsDeadChannel (
	InttMap::Offline_s const& ofl
) const {
	if(!m_HotChannelSet)return false;
	return m_HotChannelSet->find(ofl) != m_HotChannelSet->end();
}
