#!/usr/bin/env bash
set -ex

if (( ! IS_LOCAL )); then
  function output_source_preamble {
    echo "" >> ${2}
    echo "${1}###############################################################################" >> ${2}
    echo "${1}## Source: serenity.post-render.in.sh " >> ${2}
  }

  output_source_preamble "  " {{RENDER_AREA_DIR}}/serenity-service.yaml
  output_source_preamble "          " {{RENDER_AREA_DIR}}/serenity-statefulset.yaml

  for (( p={{ESTATE_WORKER_PROCESS_PORT_START}}; p<={{ESTATE_WORKER_PROCESS_PORT_END}}; p++ ))
  do
    # Update serenity-service.yaml
	  echo "  - name: ws${p}" >> {{RENDER_AREA_DIR}}/serenity-service.yaml
	  echo '    protocol: TCP' >> {{RENDER_AREA_DIR}}/serenity-service.yaml
    echo "    port: ${p}" >> {{RENDER_AREA_DIR}}/serenity-service.yaml
    # Update serenity-statefulset.yaml
    echo "          - containerPort: ${p}" >> {{RENDER_AREA_DIR}}/serenity-statefulset.yaml
    echo "            name: ws${p}" >> {{RENDER_AREA_DIR}}/serenity-statefulset.yaml
  done
fi
