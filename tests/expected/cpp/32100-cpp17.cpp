bool CompareGenomeByFeatureResults::clickOnLink(std::string const& inLink) {
	auto const [sequence, type, firstPosition, lastPosition] = parseLink(inLink);
	if (sequence.empty()) {
		return true;
	}
	return showFeature(statistics.nameDocumentA, type, firstPosition, lastPosition);
}

