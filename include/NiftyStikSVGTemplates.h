#pragma once

namespace Nifty::SVG::Templates::Fretboard
{
const std::string svg_template = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 2750 100" width="2750" height="100">
	<g transform="scale(0.1)">
		<defs>
CLIP_DATA
		</defs>
		<path d="
			M 250 750
			v -303
			h 250
			v 53
FRET_DATA
			v 250
			Z"
			fill="white"
			stroke="none"
			stroke-width="0"
SHOW_CLIP_DATA
		/>
	</g>
</svg>  
)SVG";

const std::string fret_body_template = R"(
			h FRET_LENGTH
			a FRET_WIDTH_HALF FRET_HEIGHT 0 0 1 FRET_WIDTH 0
)";

const std::string fret_body_final_template = R"(
			h FRET_LENGTH
)";

const std::string clip_template = R"(
			<clipPath id = "CLIP_ID">
				<path d="
CLIP_HEAD
CLIP_BODY
CLIP_TAIL
					fill="none"
					stroke="none"
					stroke-width="0"
				/>
			</clipPath>
)";

const std::string route_clip_head_template = R"(
					M 27500 1000
					H 0
					V 0
					H ROUTER_OFFSET_HEAD
)";

const std::string route_clip_body_template = R"(
					h ROUTER_X_OFFSET
					v ROUTER_Y_OFFSET
					a ROUTER_BIT_RADIUS ROUTER_BIT_RADIUS 0 0 0 ROUTER_BIT_DIAMETER 0
					V 0
)";

const std::string route_clip_tail_template = R"(
					H 27500
					Z"
)";

const std::string scallop_clip_id = "clip-scallop";

const std::string scallop_clip_head_template = R"(
					M 27500 1000
					H 0
					V 0
					H SCALLOP_OFFSET
					V 500
)";

const std::string scallop_clip_body_template = R"(
					a FRET_LENGTH_HALF SCALLOP_DEPTH 0 0 0 FRET_LENGTH 0
					V 0
					h FRET_WIDTH
					V 500
)";

const std::string scallop_clip_body_final_template = R"(
					a FRET_LENGTH_HALF SCALLOP_DEPTH 0 0 0 FRET_LENGTH 0
					V 0
)";

const std::string scallop_clip_tail_template = R"(
					H 27500
					Z"
)";

const std::string show_clip_template = R"SVG(
			clip-path="url(#CLIP_ID)"
)SVG";
}
