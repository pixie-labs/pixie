import { Options } from 'vis-network/standalone';
import { Theme } from '@material-ui/core/styles';

import { DataType, Relation, SemanticType } from 'types/generated/vizierapi_pb';
import * as podSVG from './pod.svg';
import * as svcSVG from './svc.svg';

export const LABEL_OPTIONS = {
  label: {
    min: 8,
    max: 20,
    maxVisible: 20,
  },
};

export interface ColInfo {
  type: DataType;
  semType: SemanticType;
  name: string;
}

export function getGraphOptions(theme: Theme, edgeLength: number): Options {
  return {
    clickToUse: true,
    layout: {
      randomSeed: 10,
      improvedLayout: false,
    },
    physics: {
      solver: 'forceAtlas2Based',
      forceAtlas2Based: {
        gravitationalConstant: -50,
        springLength: edgeLength > 0 ? edgeLength : 100,
      },
      hierarchicalRepulsion: {
        nodeDistance: 100,
      },
      stabilization: {
        iterations: 250,
        updateInterval: 10,
      },
    },
    edges: {
      smooth: false,
      scaling: {
        max: 5,
      },
      arrows: {
        to: {
          enabled: true,
          type: 'arrow',
          scaleFactor: 0.5,
        },
      },
    },
    nodes: {
      borderWidth: 0.5,
      scaling: LABEL_OPTIONS,
      font: {
        face: 'Roboto',
        color: theme.palette.text.primary,
        align: 'left',
      },
    },
  };
}

const semTypeToIcon = {
  [SemanticType.ST_SERVICE_NAME]: svcSVG,
  [SemanticType.ST_POD_NAME]: podSVG,
};

export function semTypeToShapeConfig(st: SemanticType): any {
  if (semTypeToIcon[st]) {
    const icon = semTypeToIcon[st];
    return {
      shape: 'image',
      image: {
        selected: icon,
        unselected: icon,
      },
    };
  }
  return {
    shape: 'dot',
  };
}

export function getNamespaceFromEntityName(val: string): string {
  return val.split('/')[0];
}

export function getColorForLatency(val: number, theme: Theme): string {
  if (val < 100) {
    return theme.palette.success.dark;
  }
  return val > 200 ? theme.palette.error.main : theme.palette.warning.main;
}

export function getColorForErrorRate(val: number, theme: Theme): string {
  if (val < 1) {
    return theme.palette.success.dark;
  }
  return val > 2 ? theme.palette.error.main : theme.palette.warning.main;
}

export function colInfoFromName(relation: Relation, name: string): ColInfo {
  const cols = relation.getColumnsList();
  for (let i = 0; i < cols.length; i++) {
    if (cols[i].getColumnName() === name) {
      return {
        name,
        type: cols[i].getColumnType(),
        semType: cols[i].getColumnSemanticType(),
      };
    }
  }
  return undefined;
}