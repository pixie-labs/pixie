import * as React from 'react';
import { PropsWithChildren } from 'react';
import { createStyles, makeStyles, Theme } from '@material-ui/core/styles';
import Tooltip from '@material-ui/core/Tooltip';
import { Dialog } from '@material-ui/core';
import CloseButton from '@material-ui/icons/Close';
import IconButton from '@material-ui/core/IconButton';
import 'typeface-walter-turncoat';
import { Spacing } from '@material-ui/core/styles/createSpacing';
import { SetStateFunc } from 'context/common';
import { CONTACT_ENABLED } from 'containers/constants';

export interface LiveTourContextProps {
  tourOpen: boolean;
  setTourOpen: SetStateFunc<boolean>;
}
export const LiveTourContext = React.createContext<LiveTourContextProps>({
  tourOpen: false,
  setTourOpen: () => {},
});
export const LiveTourContextProvider = ({ children }: PropsWithChildren<{}>) => {
  const [tourOpen, setTourOpen] = React.useState<boolean>(false);
  return <LiveTourContext.Provider value={{ tourOpen, setTourOpen }}>{children}</LiveTourContext.Provider>;
};

/**
 * Generates the CSS properties needed to punch holes in the backdrop
 * where the described components are, to make them easy to notice
 */
function buildBackdropMask(s: Spacing) {
  const commonMaskShape = ''
      + '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" preserveAspectRatio="none">'
      + '<rect width="100" height="100" fill="black"></rect>'
      + '</svg>';

  // x, y, w, h.
  const punchouts: string[][] = [
    /* eslint-disable no-multi-spaces */
    [`${s(9)}px`, `${s(3)}px`,                `calc(100vw - ${s(12)}px)`, `${s(4.5)}px`], // Top bar
    ['left',      `${s(8)}px`,                `${s(6)}px`,                `${s(12)}px`],  // Upper part of sidebar
    ['left',      `calc(100vh - ${s(36)}px)`, `${s(6)}px`,                `${s(36)}px`],  // Bottom part of sidebar
    [`${s(6)}px`, `calc(100vh - ${s(6)}px)`,  '100vw',                    `${s(6)}px`],   // Drawer
    /* eslint-enable no-multi-spaces */
  ];
  // Draws white everywhere except the punchout locations. Without this, the
  // exclude composite mode doesn't know what to do and applies no mask at all.
  const outside = 'linear-gradient(#fff,#fff)';

  return {
    maskImage: Array(punchouts.length)
      .fill(`url('data:image/svg+xml;utf8,${commonMaskShape}')`)
      .concat(outside)
      .join(','),
    maskPosition: punchouts.map((pos) => `${pos[0]} ${pos[1]}`).concat('0 0').join(','),
    maskSize: punchouts.map((pos) => `${pos[2]} ${pos[3]}`).concat('100% 100%').join(','),
    // Makes the mask draw only what ISN'T covered in black, rather than the opposite.
    maskComposite: 'exclude',
    maskRepeat: 'no-repeat',
  };
}

const useStyles = makeStyles((theme: Theme) => createStyles({
  tourModal: {
    // Dialog already uses a shaded background; we don't need another solid one in front of it.
    background: 'transparent',
  },
  tourModalBackdrop: {
    // Said shaded background. Punches a few holes to show the elements the modal points to.
    position: 'absolute',
    top: 0,
    left: 0,
    width: '100vw',
    height: '100vh',
    background: 'rgba(32, 32, 32, 0.8)',
    ...buildBackdropMask(theme.spacing),
  },
  closeButton: {
    position: 'absolute',
    top: theme.spacing(1),
    left: theme.spacing(1),
    zIndex: 1,
    background: 'black',
    color: 'white',
    border: '1px rgba(255, 255, 255, 0.2) solid',
  },
  content: {
    width: '100%',
    height: '100%',
    position: 'relative',

    ...theme.typography.body1,
    color: 'white',
    fontFamily: '"Walter Turncoat", cursive',

    '& h3': {
      font: theme.typography.h3.font,
      color: theme.palette.primary.main,
      marginTop: 0,
      marginBottom: theme.spacing(1),
    },

    '& p': {
      margin: 0,
    },

    '& > *': {
      position: 'absolute',
      margin: 0,
    },
  },
  topBarLeft: {
    top: theme.spacing(16),
    left: theme.spacing(52),
  },
  topBarLeftArrow: {
    position: 'absolute',
    top: theme.spacing(3),
    left: 0,
    transform: 'translate(-100%, -100%)',
  },
  topBarRight: {
    top: theme.spacing(16),
    right: theme.spacing(16),
    textAlign: 'right',
  },
  topBarRightArrow: {
    position: 'absolute',
    top: theme.spacing(3),
    right: 0,
    transform: 'translate(100%, -100%)',
  },
  upperSideBar: {
    top: theme.spacing(16),
    left: theme.spacing(16),
  },
  upperSidebarArrow: {
    position: 'absolute',
    top: theme.spacing(3),
    left: 0,
    transform: 'translate(-100%, -100%)',
  },
  lowerSideBar: {
    bottom: theme.spacing(12),
    left: theme.spacing(16),
  },
  lowerSideBarArrow: {
    position: 'absolute',
    top: theme.spacing(2.5),
    left: 0,
    transform: 'translate(-100%, -100%)',
  },
  dataDrawer: {
    bottom: theme.spacing(10),
    right: theme.spacing(16),
    textAlign: 'right',
  },
  dataDrawerArrow: {
    position: 'absolute',
    right: 0,
    top: theme.spacing(1),
    transform: 'translate(100%, 0)',
  },
}));

interface LiveTourArrowProps {
  width: number;
  height: number;
  tipX: number;
  tipY: number;
  // The tip is drawn pointing northwest.
  tipAngleDeg: number;
  path: string;
  pointLength?: number;
  className: string;
}

const LiveTourArrow: React.FC<LiveTourArrowProps> = ({
  path,
  width,
  height,
  tipX,
  tipY,
  tipAngleDeg,
  pointLength = 10,
  className,
}) => {
  const tipPath = `M ${tipX + pointLength} ${tipY} h -${pointLength} v ${pointLength}`;
  return (
    <svg width={width} height={height} className={className} xmlns='http://www.w3.org/2000/svg'>
      <path d={path} stroke='white' fill='transparent' strokeWidth={2} strokeLinecap='round' />
      <path
        d={tipPath}
        transform={`rotate(${tipAngleDeg},${tipX},${tipY})`}
        stroke='white'
        fill='transparent'
        strokeWidth={2}
        strokeLinecap='round'
      />
    </svg>
  );
};

/**
 * Contents of the tour for Live pages. Meant to exist within a fullscreen MUIDialog.
 * @constructor
 */
export const LiveTour = () => {
  const classes = useStyles();
  // TODO(nick): On mobile, the layout is entirely different (and as of this writing, core functionality is unavailable)
  //  For now, this is hidden for those users (the component to open this hides).
  return (
    <div className={classes.content}>
      <div className={classes.upperSideBar}>
        <LiveTourArrow
          path='M 10 10 Q 40 10, 40 30 T 70 50'
          tipX={10}
          tipY={10}
          tipAngleDeg={-45}
          pointLength={5}
          className={classes.upperSidebarArrow}
          width={80}
          height={60}
        />
        <h3>Navigation</h3>
        <p>Navigate to K8s objects.</p>
      </div>
      <div className={classes.topBarLeft}>
        <LiveTourArrow
          path='M 80 80 Q 10 80, 10 10'
          tipX={10}
          tipY={10}
          tipAngleDeg={45}
          className={classes.topBarLeftArrow}
          width={90}
          height={90}
        />
        <h3>Select cluster, script, & optional parameters</h3>
        <p>Display your cluster&apos;s data by running PxL scripts.</p>
      </div>
      <div className={classes.topBarRight}>
        <LiveTourArrow
          path='M 10 80 Q 80 80, 80 10'
          tipX={80}
          tipY={10}
          tipAngleDeg={45}
          className={classes.topBarRightArrow}
          width={90}
          height={90}
        />
        <h3>Run & Edit Scripts</h3>
        <p>Run with Ctrl/Cmd+Enter.</p>
        <p>Edit with Ctrl/Cmd+E.</p>
      </div>
      <div className={classes.lowerSideBar}>
        <LiveTourArrow
          path='M 5 5 L 65 5'
          tipX={5}
          tipY={5}
          tipAngleDeg={-45}
          className={classes.lowerSideBarArrow}
          width={75}
          height={10}
        />
        <h3>Pixie Info / Settings</h3>
        <div>Docs</div>
        { CONTACT_ENABLED
          && <div>Help</div>}
        <div>Admin / Cluster Status</div>
      </div>
      <div className={classes.dataDrawer}>
        <LiveTourArrow
          path='M 10 10 Q 80 10, 80 80'
          tipX={80}
          tipY={80}
          tipAngleDeg={225}
          className={classes.dataDrawerArrow}
          width={90}
          height={90}
        />
        <h3>Data Drawer</h3>
        <p>Inspect the raw data (Ctrl/Cmd+D).</p>
      </div>
    </div>
  );
};

const LiveTourBackdrop = () => {
  const classes = useStyles();
  return <div className={classes.tourModalBackdrop} />;
};

interface LiveTourDialogProps {
  onClose: () => void;
}

export const LiveTourDialog = ({ onClose }: LiveTourDialogProps) => {
  const classes = useStyles();
  const { tourOpen } = React.useContext(LiveTourContext);
  return (
    <Dialog
      open={tourOpen}
      onClose={onClose}
      classes={{
        paperFullScreen: classes.tourModal,
      }}
      BackdropComponent={LiveTourBackdrop}
      fullScreen
    >
      <Tooltip title='Close (Esc)'>
        <IconButton onClick={onClose} className={classes.closeButton}>
          <CloseButton />
        </IconButton>
      </Tooltip>
      <LiveTour />
    </Dialog>
  );
};